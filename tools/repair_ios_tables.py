"""Make Konami's Android dt870 live-data CPK readable by the iOS 11.1.1 client.

The mod installs Konami's Android dt870 (renamed _mobile_ios). A few of its tables use a different
record layout from the one the iOS client reads, so this tool rewrites those tables and nothing else.

Table steps (applied in this order; each one is a function raw table bytes -> (new raw bytes, info)):
  * Playstyle.bin, PlayerSkill.bin "compact_legacy_records": legacy named records become 4-byte records.
    pesdb::Manager record-size table at 0x105784d38, entries 42 and 43 = 4;
    LinkPesdb::ImportPlayStyleData at 0x104ba1048 advances the source by 4.
  * Player.bin "ios_national_card_flag": in every 400-byte record, bit 626 := bit 618.
    Android dt870 keeps the national-team-card flag at bit 618 and Low Screamer at bit 626.
    tmpdb::LinkPesdb::SetPlayerData copies record bit 626 (rec+0x48 bit 50) into tmpdb::Player+0x198
    bit 53 (0x104b9c318..0x104b9c338), and MyClubGamePlayer::GetBaseTeamId (0x1047e8408) shows the
    national team only when that bit is set. Every other bit of the record is kept.

To add a table change (for example the box-preview skill ids), write a function raw -> (raw, info)
and append a TableStep to the newest entry of PIPELINES (several steps may change the same table;
they run in order on its decoded bytes). Once a build ships, leave its entry as it is, record its
sha256 in SHIPPED and put later changes in a new entry: older entries are how the tool recomputes
the files earlier builds installed, which the installer may then replace (table-fixes.json
"replaces_sha256").

Every other CPK byte is preserved. A table that no longer fits its old space is given a bigger one
and the files after it move by whole 2048-byte blocks; only the numeric TOC/header fields that
describe sizes and offsets change, in place, in their original encrypted encoding.
Input and output are local user-owned assets, never repository fixtures.
"""
import argparse
import hashlib
import json
import struct
import zlib
from collections import namedtuple
from pathlib import Path
from cpk_toc import decrypt_utf, utf_table

PESDB_DIR = "common/etc/pesdb"


# ---------------------------------------------------------------- WESYS containers

def unpack_wesys(data):
    if data[3:8] != b"WESYS":
        raise ValueError("not WESYS")
    compressed, size = struct.unpack_from("<II", data, 8)
    payload = bytearray(data[16:16 + compressed])
    if len(payload) != compressed:
        raise ValueError("truncated WESYS")
    if data[2] & 2:
        if data[1] & 15 != 2:
            raise ValueError("unsupported WESYS crypt version")
        # sys::File::PostProcessMainFunc, iOS 11.1.1 0x104b7b4e4..588.
        x, y, z = 0xed5b2960, 0x4a523b4e, 0xf3a31bad
        w = (compressed | size << 16) & 0xffffffff
        for p in range(0, compressed // 4 * 4, 4):
            t = (x ^ x << 11) & 0xffffffff
            x, y, z = y, z, w
            w = (w ^ w >> 19 ^ t ^ t >> 8) & 0xffffffff
            struct.pack_into("<I", payload, p, struct.unpack_from("<I", payload, p)[0] ^ w)
    raw = zlib.decompress(payload) if data[2] & 1 else bytes(payload)
    if len(raw) != size:
        raise ValueError("WESYS size mismatch")
    return raw


def pack_wesys(raw):
    # Native WESYS explicitly supports compressed, unencrypted payloads.
    payload = zlib.compress(raw, 9)
    return b"\x00\x01\x01WESYS" + struct.pack("<II", len(payload), len(raw)) + payload


# ---------------------------------------------------------------- table steps

def compact_table(raw):
    # Legacy records contain a 4-byte bitfield followed by localized names.
    # Playstyle.bin has 168 bytes per record (36 records).
    # PlayerSkill.bin has 104 bytes per record (72 records).
    # Both tables use 4-byte records on iOS 11.1.1 (pesdb::Manager entries 42 and 43).
    if not raw:
        return raw
    candidate_matched = False
    for stride in (168, 104):
        if len(raw) % stride == 0:
            count = len(raw) // stride
            if 30 <= count <= 100:
                candidate_matched = True
                records = [raw[i * stride:(i + 1) * stride] for i in range(count)]
                ids63 = [struct.unpack_from("<I", r)[0] & 63 for r in records]
                ids127 = [struct.unpack_from("<I", r)[0] & 127 for r in records]
                ids255 = [struct.unpack_from("<I", r)[0] & 255 for r in records]
                target = list(range(count))
                if ids63 == target or ids127 == target or ids255 == target:
                    return b"".join(r[:4] for r in records)
    if candidate_matched:
        raise ValueError("unrecognized legacy table IDs")
    return raw


def compact_legacy_records(raw):
    new = compact_table(raw)
    if new == raw:
        return raw, {}
    return new, {"before": len(raw), "after": len(new), "records": len(new) // 4}


PLAYER_RECORD = 400
ANDROID_NATIONAL_CARD_BIT = 618  # Android dt870 Player.bin: national-team card
IOS_NATIONAL_CARD_BIT = 626      # iOS reads it here (rec+0x48 bit 50); Android keeps Low Screamer here


def _bit(record, n):
    return record[n >> 3] >> (n & 7) & 1


def ios_national_card_flag(raw):
    """Copy the Android national-card bit into the bit the iOS client reads. Returns (raw, counts)."""
    if not raw or len(raw) % PLAYER_RECORD:
        raise ValueError("Player.bin is not a whole number of 400-byte records")
    out = bytearray(raw)
    byte, mask = IOS_NATIONAL_CARD_BIT >> 3, 1 << (IOS_NATIONAL_CARD_BIT & 7)
    counts = {"records": len(raw) // PLAYER_RECORD, "bit626_set": 0, "bit626_cleared": 0, "unchanged": 0}
    for start in range(0, len(raw), PLAYER_RECORD):
        flag = _bit(raw[start:start + PLAYER_RECORD], ANDROID_NATIONAL_CARD_BIT)
        old = _bit(raw[start:start + PLAYER_RECORD], IOS_NATIONAL_CARD_BIT)
        if flag == old:
            counts["unchanged"] += 1
            continue
        if flag:
            out[start + byte] |= mask
            counts["bit626_set"] += 1
        else:
            out[start + byte] &= ~mask & 0xff
            counts["bit626_cleared"] += 1
    return bytes(out), counts


TableStep = namedtuple("TableStep", "table step function")

COMPACT_STEPS = (
    TableStep("Playstyle.bin", "compact_legacy_records", compact_legacy_records),
    TableStep("PlayerSkill.bin", "compact_legacy_records", compact_legacy_records),
)

# Each entry is one shipped (or about to ship) version of the bundled dt870: (name, steps).
# The last entry is what the tool builds. Earlier entries reproduce what older builds installed.
PIPELINES = (
    ("build138-fix2", COMPACT_STEPS),  # shipped from build 138 to fix2: sha256 8a2a7725...
    ("fix3", COMPACT_STEPS + (
        TableStep("Player.bin", "ios_national_card_flag", ios_national_card_flag),
    )),
)
TABLE_STEPS = PIPELINES[-1][1]

# What earlier builds really shipped, for Konami's dt870 r146343 (sha256 fce1f90f...). Only used to check that
# this tool still reproduces those files byte for byte (same zlib output), so the installer will recognise them.
SHIPPED = {
    "fce1f90f65e48c4f99c5654a4ca56c917663d6033f6adc16a9014f5786a93259": {
        "build138-fix2": "8a2a77254d5b86f35feb2111eb24a689c5b362167eac2af8a07c24318a2136ff",
    },
}


# ---------------------------------------------------------------- CPK (CRI @UTF) in-place field edits

_UTF_FORMATS = {0: ">B", 1: ">b", 2: ">H", 3: ">h", 4: ">I", 5: ">i", 6: ">Q", 7: ">q", 8: ">f", 0xa: ">I", 0xb: ">II"}


def _utf_field_positions(plain):
    """Byte positions of every per-row (storage 0x50) value in a decrypted @UTF packet:
    [{column: (offset in packet, struct format)} per row]."""
    if plain[:4] != b"@UTF":
        raise ValueError("no @UTF")
    t = 8
    rows_off, str_off = struct.unpack_from(">II", plain, t)
    ncols, rowlen, nrows = struct.unpack_from(">HHI", plain, t + 16)

    def text(o):
        s = t + str_off + o
        return plain[s:plain.index(b"\0", s)].decode("utf-8")
    columns, p = [], t + 24
    for _ in range(ncols):
        flags = plain[p]
        name = text(struct.unpack_from(">I", plain, p + 1)[0])
        p += 5
        storage, typ = flags & 0xf0, flags & 0x0f
        if storage == 0x30:
            p += struct.calcsize(_UTF_FORMATS[typ])
        columns.append((name, storage, typ))
    rows = []
    for r in range(nrows):
        q = t + rows_off + r * rowlen
        row = {}
        for name, storage, typ in columns:
            if storage == 0x50:
                row[name] = (q, _UTF_FORMATS[typ])
                q += struct.calcsize(_UTF_FORMATS[typ])
            else:
                row[name] = (None, storage)
        rows.append(row)
    return rows


def _patch_utf(packet, edits):
    """edits: {(row index, column): new int}. Keeps the packet's encryption and every other byte."""
    encrypted = packet[:4] != b"@UTF"
    plain = bytearray(decrypt_utf(packet) if encrypted else packet)
    positions = _utf_field_positions(bytes(plain))
    for (row, column), value in edits.items():
        offset, fmt = positions[row][column]
        if offset is None:
            raise ValueError("CPK column %s is not stored per row" % column)
        struct.pack_into(fmt, plain, offset, value)
    return decrypt_utf(bytes(plain)) if encrypted else bytes(plain)


def read_cpk(data):
    """(header dict, header packet span, toc rows, toc packet span, base) of a CRI CPK."""
    if data[:4] != b"CPK ":
        raise ValueError("not CPK")
    header_len = struct.unpack_from("<Q", data, 8)[0]
    header = utf_table(data[16:16 + header_len])[0]
    for key in ("HtocOffset", "EtocOffset", "ItocOffset", "GtocOffset", "HgtocOffset"):
        if header.get(key):
            raise ValueError("unsupported CPK feature " + key)
    if header.get("EnableTocCrc") or header.get("EnableFileCrc") or header.get("CrcMode"):
        raise ValueError("CPK CRCs are not supported")
    toc = header["TocOffset"]
    if data[toc:toc + 4] != b"TOC ":
        raise ValueError("no TOC")
    toc_len = struct.unpack_from("<Q", data, toc + 8)[0]
    rows = utf_table(data[toc + 16:toc + 16 + toc_len])
    base = min(toc, header["ContentOffset"])
    return header, (16, 16 + header_len), rows, (toc + 16, toc + 16 + toc_len), base


def _align(n, a):
    return (n + a - 1) // a * a


def _find_row(rows, table):
    hits = [i for i, r in enumerate(rows) if r.get("DirName") == PESDB_DIR and r.get("FileName") == table]
    if len(hits) != 1:
        raise ValueError("%s/%s found %d times in the CPK" % (PESDB_DIR, table, len(hits)))
    return hits[0]


def repair_cpk_bytes(data, steps=None):
    """Apply table steps to a dt870 CPK. Returns (new CPK bytes, list of change records)."""
    steps = TABLE_STEPS if steps is None else steps
    header, (h0, h1), rows, (t0, t1), base = read_cpk(data)
    align = header.get("Align") or 2048
    order = sorted(range(len(rows)), key=lambda i: rows[i]["FileOffset"])
    for a, b in zip(order, order[1:]):
        if rows[a]["FileOffset"] + rows[a]["FileSize"] > rows[b]["FileOffset"]:
            raise ValueError("overlapping CPK entries")

    tables, changes = {}, []
    for step in steps:
        i = _find_row(rows, step.table)
        row = rows[i]
        if row["FileSize"] != row["ExtractSize"]:
            raise ValueError("CPK-compressed table unsupported")
        if i not in tables:
            start = base + row["FileOffset"]
            tables[i] = unpack_wesys(data[start:start + row["FileSize"]])
        new, info = step.function(tables[i])
        if new != tables[i] or info:
            changes.append(dict({"table": step.table, "step": step.step}, **info))
        tables[i] = new

    # New payloads: unchanged tables keep their exact bytes.
    payload = {}
    for i, raw in tables.items():
        start = base + rows[i]["FileOffset"]
        if unpack_wesys(data[start:start + rows[i]["FileSize"]]) != raw:
            payload[i] = pack_wesys(raw)
    if not payload:
        return bytes(data), changes

    # Layout: a table that fits keeps its place and size field (zero padded, as build 138 .. fix2 did).
    # A bigger one gets a bigger slot; later files move by whole alignment blocks.
    new_offset, new_size, shift = {}, {}, 0
    for k, i in enumerate(order):
        row = rows[i]
        new_offset[i] = row["FileOffset"] + shift
        size = row["FileSize"]
        if i in payload and len(payload[i]) > size:
            size = len(payload[i])
        new_size[i] = size
        if k + 1 < len(order):
            nxt = rows[order[k + 1]]["FileOffset"] + shift
            need = _align(new_offset[i] + size + base, align) - base
            if need > nxt:
                shift += need - nxt
    if (base + rows[order[0]]["FileOffset"]) % align or shift % align:
        raise ValueError("CPK alignment broken")

    # Each file keeps its slot: its bytes plus the packer's filler up to the next file (the filler is
    # not zero in Konami's CPK and is copied unchanged). Only a grown table gets a new, zero-filled tail.
    out = bytearray(data[:base + rows[order[0]]["FileOffset"]])
    for k, i in enumerate(order):
        old = base + rows[i]["FileOffset"]
        slot_end = base + rows[order[k + 1]]["FileOffset"] if k + 1 < len(order) else len(data)
        out += b"\0" * (base + new_offset[i] - len(out))
        if i in payload and new_size[i] == rows[i]["FileSize"]:
            slot = bytearray(data[old:slot_end])
            slot[:new_size[i]] = payload[i].ljust(new_size[i], b"\0")
            out += slot
        elif i in payload:
            out += payload[i]
        else:
            out += data[old:slot_end]

    if shift or any(new_size[i] != rows[i]["FileSize"] for i in range(len(rows))):
        toc_edits = {}
        for i in range(len(rows)):
            if new_offset[i] != rows[i]["FileOffset"]:
                toc_edits[(i, "FileOffset")] = new_offset[i]
            if new_size[i] != rows[i]["FileSize"]:
                toc_edits[(i, "FileSize")] = new_size[i]
                toc_edits[(i, "ExtractSize")] = new_size[i]
        out[t0:t1] = _patch_utf(bytes(data[t0:t1]), toc_edits)
        # Header totals move by exactly what changed, so whatever convention the packer used is kept
        # (Konami's: ContentOffset + ContentSize = end of the last file rounded up to the alignment).
        last = order[-1]
        grown = sum(new_size[i] - rows[i]["FileSize"] for i in range(len(rows)))
        content_delta = (_align(base + new_offset[last] + new_size[last], align)
                         - _align(base + rows[last]["FileOffset"] + rows[last]["FileSize"], align))
        header_edits = {
            (0, "ContentSize"): header["ContentSize"] + content_delta,
            (0, "EnabledPackedSize"): header["EnabledPackedSize"] + grown,
            (0, "EnabledDataSize"): header["EnabledDataSize"] + grown,
        }
        out[h0:h1] = _patch_utf(bytes(data[h0:h1]), header_edits)
        for i in sorted(payload, key=lambda i: rows[i]["FileOffset"]):
            if new_size[i] != rows[i]["FileSize"]:
                changes.append({"table": rows[i]["FileName"], "step": "cpk_layout", "file_size_before": rows[i]["FileSize"],
                                "file_size_after": new_size[i], "cpk_size_before": len(data), "cpk_size_after": len(out)})
    return bytes(out), changes


def verify_cpk(original, fixed, steps=None):
    """Independent check of a repaired CPK against its source. Raises on any problem; returns a report."""
    steps = TABLE_STEPS if steps is None else steps
    h0, _, r0, _, b0 = read_cpk(original)
    h1, _, r1, _, b1 = read_cpk(fixed)
    if b0 != b1 or len(r0) != len(r1):
        raise ValueError("CPK layout base or file count changed")
    for key in h0:
        if key not in ("ContentSize", "EnabledPackedSize", "EnabledDataSize") and h0[key] != h1[key]:
            raise ValueError("CPK header field changed: " + key)
    align = h1.get("Align") or 2048
    expected = {}
    for step in steps:
        i = _find_row(r0, step.table)
        if i not in expected:
            s = b0 + r0[i]["FileOffset"]
            expected[i] = unpack_wesys(original[s:s + r0[i]["FileSize"]])
        expected[i] = step.function(expected[i])[0]
    changed = []
    for i, (a, b) in enumerate(zip(r0, r1)):
        for key in a:
            if key not in ("FileOffset", "FileSize", "ExtractSize") and a[key] != b[key]:
                raise ValueError("TOC field changed: %s %s" % (a.get("FileName"), key))
        if b["FileSize"] != b["ExtractSize"] or (b1 + b["FileOffset"]) % align:
            raise ValueError("bad size or alignment: " + b["FileName"])
        old = original[b0 + a["FileOffset"]:b0 + a["FileOffset"] + a["FileSize"]]
        new = fixed[b1 + b["FileOffset"]:b1 + b["FileOffset"] + b["FileSize"]]
        if i in expected:
            if unpack_wesys(new) != expected[i]:
                raise ValueError("table does not decode to the expected bytes: " + b["FileName"])
            if new != old:
                changed.append(b["FileName"])
        elif new != old:
            raise ValueError("untouched file changed: %s/%s" % (b["DirName"], b["FileName"]))
    spans = sorted((b["FileOffset"], b["FileOffset"] + b["FileSize"]) for b in r1)
    for (s0, e0), (s1, _) in zip(spans, spans[1:]):
        if e0 > s1:
            raise ValueError("overlapping files")
    end = b1 + spans[-1][1]
    if len(fixed) != end:
        raise ValueError("CPK does not end at its last file")
    if h1["ContentOffset"] + h1["ContentSize"] != _align(end, align):
        raise ValueError("ContentSize does not cover the files")
    if h1["EnabledPackedSize"] != sum(b["FileSize"] for b in r1) or h1["EnabledDataSize"] != sum(b["ExtractSize"] for b in r1):
        raise ValueError("Enabled sizes do not match the TOC")
    return {"files": len(r1), "changed_files": changed, "size": len(fixed)}


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def table_fix_entry(original, pipelines=PIPELINES):
    """Build the newest repaired CPK and its table-fixes.json entry. The entry names every file an
    earlier build may have installed (Konami's original and the output of each older pipeline), all
    computed here from the same original, so the installer can replace them."""
    outputs = []
    for name, steps in pipelines:
        fixed, changes = repair_cpk_bytes(original, steps)
        verify_cpk(original, fixed, steps)
        outputs.append((name, fixed, changes))
    for name, older, _ in outputs:
        want = SHIPPED.get(sha256(original), {}).get(name)
        if want and sha256(older) != want:
            raise ValueError("pipeline %s no longer reproduces the shipped file %s (zlib changed?)" % (name, want))
    name, fixed, changes = outputs[-1]
    replaces = [sha256(original)]
    for _, older, _ in outputs[:-1]:
        h = sha256(older)
        if h not in replaces and older != fixed:
            replaces.append(h)
    entry = {"original_sha256": sha256(original), "sha256": sha256(fixed), "size": len(fixed),
             "pipeline": name, "replaces_sha256": replaces, "changes": changes}
    return fixed, entry


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("input", type=Path, help="Konami's original dt870_mobile_and.cpk")
    ap.add_argument("output", type=Path)
    ap.add_argument("--manifest", type=Path, help="write the table-fixes.json for this output here")
    args = ap.parse_args()
    if args.output.resolve() == args.input.resolve():
        raise SystemExit("use a separate output path")
    original = args.input.read_bytes()
    fixed, entry = table_fix_entry(original)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(fixed)
    if args.manifest:
        args.manifest.write_text(json.dumps({"dt870_mobile_and.cpk": entry}, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(entry, indent=2))
