import hashlib
import os
import random
import struct
import sys
import unittest
import zlib
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from cpk_toc import decrypt_utf, utf_table
from repair_ios_tables import (PIPELINES, TableStep, compact_table, ios_national_card_flag, read_cpk,
                               repair_cpk_bytes, table_fix_entry, unpack_wesys, verify_cpk)


class IosTablesTest(unittest.TestCase):
    def test_legacy_records_keep_attribute_and_unlock_bits(self):
        values = [i | ((i % 3) << 6) | 256 for i in range(36)]
        raw = b"".join(struct.pack("<I", n) + "Tên phong cách".encode().ljust(164, b"\0") for n in values)
        fixed = compact_table(raw)
        self.assertEqual(len(fixed), 36 * 4)
        self.assertEqual(struct.unpack("<36I", fixed), tuple(values))
        self.assertEqual(compact_table(fixed), fixed)

    def test_player_skill_104_byte_stride(self):
        values = [i | ((i % 5) << 7) | 1024 for i in range(72)]
        raw = b"".join(struct.pack("<I", n) + "Kỹ năng".encode().ljust(100, b"\0") for n in values)
        fixed = compact_table(raw)
        self.assertEqual(len(fixed), 72 * 4)
        self.assertEqual(struct.unpack("<72I", fixed), tuple(values))

    def test_unknown_legacy_ids_are_rejected(self):
        with self.assertRaises(ValueError):
            compact_table(b"\0" * (168 * 36))

    def test_native_unencrypted_wesys_payload(self):
        raw = struct.pack("<36I", *range(36))
        compressed = zlib.compress(raw)
        packed = b"\0\1\1WESYS" + struct.pack("<II", len(compressed), len(raw)) + compressed
        self.assertEqual(unpack_wesys(packed + b"\0" * 50), raw)
        with self.assertRaises(ValueError):
            unpack_wesys(packed[:-5])

    def test_konami_encrypted_wesys_payload(self):
        rng = random.Random(3)
        raw = bytes(rng.randrange(256) for _ in range(5000))
        self.assertEqual(unpack_wesys(konami_wesys(raw)), raw)


class NationalFlagTest(unittest.TestCase):
    def test_bit_618_is_copied_to_bit_626_and_nothing_else_changes(self):
        rng = random.Random(7)
        records = []
        for want_618, want_626 in [(0, 0), (1, 0), (0, 1), (1, 1)] * 5:
            rec = bytearray(rng.randrange(256) for _ in range(400))
            set_bit(rec, 618, want_618)
            set_bit(rec, 626, want_626)
            records.append(bytes(rec))
        raw = b"".join(records)
        fixed, counts = ios_national_card_flag(raw)
        self.assertEqual(counts, {"records": 20, "bit626_set": 5, "bit626_cleared": 5, "unchanged": 10})
        for i in range(20):
            old, new = raw[i * 400:(i + 1) * 400], fixed[i * 400:(i + 1) * 400]
            self.assertEqual(get_bit(new, 626), get_bit(old, 618))
            changed = [b for b in range(3200) if get_bit(old, b) != get_bit(new, b)]
            self.assertIn(changed, ([], [626]))
        again, counts = ios_national_card_flag(fixed)
        self.assertEqual(again, fixed)
        self.assertEqual(counts["unchanged"], 20)

    def test_partial_records_are_rejected(self):
        with self.assertRaises(ValueError):
            ios_national_card_flag(b"\0" * 401)
        with self.assertRaises(ValueError):
            ios_national_card_flag(b"")


class CpkRepairTest(unittest.TestCase):
    def setUp(self):
        rng = random.Random(11)
        styles = b"".join(struct.pack("<I", i | 256) + b"Style".ljust(164, b"\0") for i in range(36))
        skills = b"".join(struct.pack("<I", i | 1024) + b"Skill".ljust(100, b"\0") for i in range(72))
        players = bytearray(rng.randrange(256) for _ in range(400 * 60))  # does not compress: grows on repack
        for i in range(60):
            set_bit(players, i * 400 * 8 + 618, i % 3 == 0)
            set_bit(players, i * 400 * 8 + 626, i % 5 == 0)
        self.players = bytes(players)
        self.files = [
            ("common/demo", "a.bin", bytes(rng.randrange(256) for _ in range(3000))),
            ("common/etc/pesdb", "Player.bin", konami_wesys(self.players)),
            ("common/etc/pesdb", "PlayerSkill.bin", konami_wesys(skills)),
            ("common/etc/pesdb", "Playstyle.bin", konami_wesys(styles)),
            ("eng/string", "z.str", bytes(rng.randrange(256) for _ in range(700))),
        ]
        self.cpk = build_cpk(self.files, rng)

    def payload(self, cpk, name):
        _, _, rows, _, base = read_cpk(cpk)
        row = [r for r in rows if r["FileName"] == name][0]
        return cpk[base + row["FileOffset"]:base + row["FileOffset"] + row["FileSize"]]

    def test_tables_that_fit_stay_in_place(self):
        fixed, changes = repair_cpk_bytes(self.cpk, PIPELINES[0][1])
        self.assertEqual(len(fixed), len(self.cpk))
        self.assertEqual([c["table"] for c in changes], ["Playstyle.bin", "PlayerSkill.bin"])
        h0, (a, b), _, (c, d), _ = read_cpk(self.cpk)
        self.assertEqual(fixed[:d], self.cpk[:d])  # header and TOC bytes untouched
        differ = [i for i in range(len(fixed)) if fixed[i] != self.cpk[i]]
        spans = [span(self.cpk, n) for n in ("PlayerSkill.bin", "Playstyle.bin")]
        self.assertTrue(all(any(s <= i < e for s, e in spans) for i in differ))
        self.assertEqual(len(unpack_wesys(self.payload(fixed, "Playstyle.bin"))), 36 * 4)
        verify_cpk(self.cpk, fixed, PIPELINES[0][1])

    def test_default_pipeline_repairs_all_three_tables(self):
        fixed, changes = repair_cpk_bytes(self.cpk)
        report = verify_cpk(self.cpk, fixed)
        self.assertEqual(sorted(report["changed_files"]), ["Player.bin", "PlayerSkill.bin", "Playstyle.bin"])
        self.assertEqual(unpack_wesys(self.payload(fixed, "Player.bin")), ios_national_card_flag(self.players)[0])
        flag = [c for c in changes if c["step"] == "ios_national_card_flag"][0]
        self.assertEqual((flag["bit626_set"], flag["bit626_cleared"]), (16, 8))
        self.assertEqual(repair_cpk_bytes(fixed)[0], fixed)  # running it again changes nothing

    def test_grown_table_moves_later_files_by_whole_blocks(self):
        rng = random.Random(5)
        extra = bytes(rng.randrange(256) for _ in range(9000))  # incompressible: the table must grow
        steps = PIPELINES[-1][1] + (TableStep("Player.bin", "test_grow", lambda raw: (raw + extra, {"added": 9000})),)
        fixed, changes = repair_cpk_bytes(self.cpk, steps)
        report = verify_cpk(self.cpk, fixed, steps)
        self.assertEqual(sorted(report["changed_files"]), ["Player.bin", "PlayerSkill.bin", "Playstyle.bin"])
        layout = [c for c in changes if c["step"] == "cpk_layout"]
        self.assertEqual([c["table"] for c in layout], ["Player.bin"])
        self.assertGreater(len(fixed), len(self.cpk))
        self.assertEqual((len(fixed) - len(self.cpk)) % 2048, 0)
        # Every file decodes; untouched files and the filler after them are byte-identical.
        expected_players = ios_national_card_flag(self.players)[0] + extra
        self.assertEqual(unpack_wesys(self.payload(fixed, "Player.bin")), expected_players)
        for d, n, data in self.files:
            if n in ("a.bin", "z.str"):
                self.assertEqual(self.payload(fixed, n), data)
        s0, e0 = span(self.cpk, "z.str", slot=True)
        s1, e1 = span(fixed, "z.str", slot=True)
        self.assertEqual(fixed[s1:e1], self.cpk[s0:e0])
        # Header and TOC stay encrypted; only their size and offset numbers change.
        for (a, b), (c, d) in zip(packets(self.cpk), packets(fixed)):
            self.assertNotEqual(self.cpk[a:a + 4], b"@UTF")
            self.assertNotEqual(fixed[c:c + 4], b"@UTF")
            self.assertEqual(b - a, d - c)
        old_rows, new_rows = read_cpk(self.cpk)[2], read_cpk(fixed)[2]
        for o, n in zip(old_rows, new_rows):
            self.assertEqual({k: v for k, v in o.items() if k not in ("FileOffset", "FileSize", "ExtractSize")},
                             {k: v for k, v in n.items() if k not in ("FileOffset", "FileSize", "ExtractSize")})
        moved = [n["FileOffset"] - o["FileOffset"] for o, n in zip(old_rows, new_rows) if n["FileName"] == "PlayerSkill.bin"]
        self.assertEqual(moved, [len(fixed) - len(self.cpk)])

    def test_manifest_entry_lists_every_earlier_output(self):
        fixed, entry = table_fix_entry(self.cpk)
        older = repair_cpk_bytes(self.cpk, PIPELINES[0][1])[0]
        self.assertEqual(entry["sha256"], sha(fixed))
        self.assertEqual(entry["original_sha256"], sha(self.cpk))
        self.assertEqual(entry["replaces_sha256"], [sha(self.cpk), sha(older)])
        self.assertEqual(entry["size"], len(fixed))

    def test_verify_catches_a_changed_untouched_file(self):
        fixed, _ = repair_cpk_bytes(self.cpk)
        s, _ = span(fixed, "z.str")
        broken = bytearray(fixed)
        broken[s + 10] ^= 1
        with self.assertRaises(ValueError):
            verify_cpk(self.cpk, bytes(broken))

    @unittest.skipUnless(os.environ.get("DT870_ORIGINAL"), "set DT870_ORIGINAL to Konami's dt870_mobile_and.cpk")
    def test_konami_dt870(self):
        original = Path(os.environ["DT870_ORIGINAL"]).read_bytes()
        self.assertEqual(sha(original), "fce1f90f65e48c4f99c5654a4ca56c917663d6033f6adc16a9014f5786a93259")
        fixed, entry = table_fix_entry(original)
        # The compaction-only pipeline reproduces the copy shipped from build 138 to fix2.
        self.assertIn("8a2a77254d5b86f35feb2111eb24a689c5b362167eac2af8a07c24318a2136ff", entry["replaces_sha256"])
        flag = [c for c in entry["changes"] if c["step"] == "ios_national_card_flag"][0]
        self.assertEqual((flag["records"], flag["bit626_set"], flag["bit626_cleared"]), (44028, 4078, 44))


# ---------------------------------------------------------------- synthetic CRI CPK writer (test only)

def sha(b):
    return hashlib.sha256(b).hexdigest()


def get_bit(buf, n):
    return buf[n >> 3] >> (n & 7) & 1


def set_bit(buf, n, value):
    if value:
        buf[n >> 3] |= 1 << (n & 7)
    else:
        buf[n >> 3] &= ~(1 << (n & 7)) & 0xff


def konami_wesys(raw):
    """Compressed and encrypted, as Konami ships tables (the cipher is a XOR stream, so it is its own inverse)."""
    payload = zlib.compress(raw, 6)
    packed = b"\xff\x22\x83WESYS" + struct.pack("<II", len(payload), len(raw)) + payload
    head = b"\x00\x01\x01WESYS" + packed[8:16]
    plain = unpack_wesys(head + payload)  # sanity: compressed stream is valid
    assert plain == raw
    x, y, z = 0xed5b2960, 0x4a523b4e, 0xf3a31bad
    w = (len(payload) | len(raw) << 16) & 0xffffffff
    out = bytearray(payload)
    for p in range(0, len(payload) // 4 * 4, 4):
        t = (x ^ x << 11) & 0xffffffff
        x, y, z = y, z, w
        w = (w ^ w >> 19 ^ t ^ t >> 8) & 0xffffffff
        struct.pack_into("<I", out, p, struct.unpack_from("<I", out, p)[0] ^ w)
    return packed[:16] + bytes(out)


FORMATS = {2: ">H", 4: ">I", 6: ">Q"}


def encode_utf(name, columns, rows):
    strings, index = bytearray(), {}

    def string(value):
        if value not in index:
            index[value] = len(strings)
            strings.extend(value.encode() + b"\0")
        return index[value]
    name_offset = string(name)
    schema, values = bytearray(), bytearray()
    for column, typ, const in columns:
        if const is not None:
            schema += bytes([0x30 | typ]) + struct.pack(">I", string(column)) + struct.pack(">I", string(const))
        else:
            schema += bytes([0x50 | typ]) + struct.pack(">I", string(column))
    for row in rows:
        for column, typ, const in columns:
            if const is None:
                values += struct.pack(">I", string(row[column])) if typ == 0xa else struct.pack(FORMATS[typ], row[column])
    rows_offset = 24 + len(schema)
    strings_offset = rows_offset + len(values)
    row_length = len(values) // len(rows)
    body = struct.pack(">IIIIHHI", rows_offset, strings_offset, strings_offset + len(strings), name_offset,
                       len(columns), row_length, len(rows)) + schema + values + strings
    return decrypt_utf(b"@UTF" + struct.pack(">I", len(body)) + body)  # encrypted like Konami's


def build_cpk(files, rng):
    toc_columns = [("DirName", 0xa, None), ("FileName", 0xa, None), ("FileSize", 4, None), ("ExtractSize", 4, None),
                   ("FileOffset", 6, None), ("ID", 4, None), ("UserString", 0xa, "<NULL>")]
    header_columns = [("UpdateDateTime", 6, None), ("ContentOffset", 6, None), ("ContentSize", 6, None),
                      ("TocOffset", 6, None), ("TocSize", 6, None), ("EnabledPackedSize", 6, None),
                      ("EnabledDataSize", 6, None), ("Files", 4, None), ("Align", 2, None), ("Sorted", 2, None),
                      ("EnableTocCrc", 2, None), ("EnableFileCrc", 2, None), ("CrcMode", 4, None)]
    align = lambda n: (n + 2047) // 2048 * 2048
    rows = [{"DirName": d, "FileName": n, "FileSize": len(b), "ExtractSize": len(b), "FileOffset": 0, "ID": i}
            for i, (d, n, b) in enumerate(files)]
    toc = encode_utf("CpkTocInfo", toc_columns, rows)
    content = align(2048 + 16 + len(toc))
    pos = content
    for row, (_, _, b) in zip(rows, files):
        row["FileOffset"] = pos - 2048
        pos = align(pos + len(b))
    toc = encode_utf("CpkTocInfo", toc_columns, rows)
    end = rows[-1]["FileOffset"] + 2048 + len(files[-1][2])
    header = encode_utf("CpkHeader", header_columns, [{
        "UpdateDateTime": 0, "ContentOffset": content, "ContentSize": align(end) - content, "TocOffset": 2048,
        "TocSize": 16 + len(toc), "EnabledPackedSize": sum(len(f[2]) for f in files),
        "EnabledDataSize": sum(len(f[2]) for f in files), "Files": len(files), "Align": 2048, "Sorted": 1,
        "EnableTocCrc": 0, "EnableFileCrc": 0, "CrcMode": 0}])
    filler = lambda n: bytes(rng.randrange(256) for _ in range(n))  # Konami's filler is not zero
    out = bytearray(b"CPK \0\0\0\0" + struct.pack("<Q", len(header)) + header)
    out += filler(2048 - len(out))
    out += b"TOC \0\0\0\0" + struct.pack("<Q", len(toc)) + toc
    out += filler(content - len(out))
    for row, (_, _, b) in zip(rows, files):
        out += filler(2048 + row["FileOffset"] - len(out))
        out += b
    return bytes(out)


def span(cpk, name, slot=False):
    _, _, rows, _, base = read_cpk(cpk)
    order = sorted(rows, key=lambda r: r["FileOffset"])
    for k, r in enumerate(order):
        if r["FileName"] == name:
            start = base + r["FileOffset"]
            if slot:
                end = base + order[k + 1]["FileOffset"] if k + 1 < len(order) else len(cpk)
                return start, end
            return start, start + r["FileSize"]
    raise KeyError(name)


def packets(cpk):
    _, h, _, t, _ = read_cpk(cpk)
    return [h, t]


if __name__ == "__main__":
    unittest.main()
