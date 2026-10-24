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
from repair_ios_tables import (PESDB_DIR, PIPELINES, SHIPPED, TableStep, compact_table, ios_national_card_flag, read_cpk,
                               repair_cpk_bytes, table_fix_entry, unpack_wesys, verify_cpk)
import ios_skill_tables as skills


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


class SkillNumberingTest(unittest.TestCase):
    """Android skill card a is iOS card a + (a >= 25) + (a >= 67); iOS 25 and 68 have no Android card."""

    def test_boundaries(self):
        self.assertEqual([skills.android_to_ios_skill(a) for a in (0, 24, 25, 26, 66, 67, 68, 71)],
                         [0, 24, 26, 27, 67, 69, 70, 73])

    def test_map_is_injective_and_skips_ios_only_cards(self):
        image = set(skills.ANDROID_TO_IOS)
        self.assertEqual(len(image), 72)
        self.assertEqual(set(range(74)) - image, set(skills.IOS_ONLY_SKILLS))

    def test_rejects_non_skills(self):
        for bad in (-1, 72, 73, 127, 1.0):
            with self.assertRaises(ValueError):
                skills.android_to_ios_skill(bad)

    def test_empty_marker(self):
        # The featured-skill empty marker is not a skill: 72 becomes 74, never iosSkillId(72) = 25 (a Dummy card).
        self.assertEqual(skills.android_to_ios_pr_slot(72), 74)
        with self.assertRaises(ValueError):
            skills.android_to_ios_pr_slot(73)


class FeaturedSkillTableTest(unittest.TestCase):
    """PlayerVariationPrSkill.bin: the featured special skills of box and banner previews."""

    def table(self):
        return (pr_row(5, 29, 72, 30) + pr_row(9, 72, 72, 72) + pr_row(12, 68, 72, 24, high=0x3ff, tail=7)
                + pr_row(40, 0, 1, 71))

    def test_converts_each_slot_and_keeps_everything_else(self):
        raw = self.table()
        out = skills.convert_pr_skill_table(raw)
        self.assertEqual(len(out), len(raw))
        rows = [struct.unpack_from("<QII", out, i) for i in range(0, len(out), 16)]
        self.assertEqual([r[0] for r in rows], [5, 9, 12, 40])
        self.assertEqual([skills.pr_slots(r[1]) for r in rows], [[30, 74, 31], [74, 74, 74], [70, 74, 24], [0, 1, 73]])
        self.assertEqual(rows[2][1] >> 21, 0x3ff)
        self.assertEqual(rows[2][2], 7)

    def test_numbering_detection_and_step_runs_once(self):
        raw = self.table()
        self.assertEqual(skills.pr_skill_numbering(raw), "android")
        once, counts = skills.renumber_featured_skills(raw)
        self.assertEqual(counts, {"records": 4, "skills_renumbered": 4, "skills_kept": 3, "empty_slots": 5})
        self.assertEqual(skills.pr_skill_numbering(once), "ios")
        self.assertEqual(skills.renumber_featured_skills(once), (once, {}))
        with self.assertRaises(ValueError):
            skills.convert_pr_skill_table(once)

    def test_ios_table_featuring_fighting_spirit_is_still_ios(self):
        # iOS 72 is a real skill (Fighting Spirit); with a 74 present the table is iOS.
        self.assertEqual(skills.pr_skill_numbering(pr_row(1, 72, 74, 74)), "ios")

    def test_unknown_numbering_is_refused(self):
        full = pr_row(1, 3, 4, 5)
        self.assertEqual(skills.pr_skill_numbering(full), "unknown")
        with self.assertRaises(ValueError):
            skills.renumber_featured_skills(full)
        odd = pr_row(1, 100, 72, 72)  # neither platform writes 75..127
        self.assertEqual(skills.pr_skill_numbering(odd), "unknown")
        with self.assertRaises(ValueError):
            skills.renumber_featured_skills(odd)

    def test_order_must_stay_sorted(self):
        with self.assertRaises(ValueError):
            skills.convert_pr_skill_table(pr_row(9, 1, 72, 72) + pr_row(5, 1, 72, 72))
        with self.assertRaises(ValueError):
            skills.convert_pr_skill_table(b"\0" * 15)

    def test_display_order_swaps_second_and_third_slot(self):
        word = struct.unpack_from("<I", skills.convert_pr_skill_table(pr_row(1, 29, 72, 30)), 8)[0]
        self.assertEqual(client_featured_skills(word), [30, 31])
        android_word = struct.unpack_from("<I", pr_row(1, 29, 72, 30), 8)[0]
        self.assertEqual(client_featured_skills(android_word), [29, 30, 72])  # what the client showed before

    def test_steps_belong_to_the_fix3_pipeline_only(self):
        # The fix2 entry must keep reproducing the shipped 8a2a7725 file.
        names = dict(PIPELINES)
        self.assertEqual([(s.table, s.step) for s in names["build138-fix2"]],
                         [("Playstyle.bin", "compact_legacy_records"), ("PlayerSkill.bin", "compact_legacy_records")])
        p3 = [(s.table, s.function) for s in names["fix3"] if s.step == "ios_skill_numbering"]
        self.assertEqual(p3, [("PlayerSkill.bin", skills.renumber_player_skills),
                              ("PlayerVariationPrSkill.bin", skills.renumber_featured_skills)])


KONAMI_DT870 = "fce1f90f65e48c4f99c5654a4ca56c917663d6033f6adc16a9014f5786a93259"   # Konami's dt870 r146343
FIX2_DT870 = "8a2a77254d5b86f35feb2111eb24a689c5b362167eac2af8a07c24318a2136ff"     # shipped from build 138 to fix2
FIX3_DT870 = "599f17f0f1a9684685b1ca5b02d3717e501949e7e0f122e2c556a41efd8331af"     # shipped in fix3


class ShippedPipelinesTest(unittest.TestCase):
    """A build that shipped keeps its PIPELINES entry exactly as it was: the tool rebuilds from it the file that
    build installed, and the installer replaces only such files (table-fixes.json replaces_sha256). A later change
    goes in a new entry."""

    FROZEN = {
        "build138-fix2": [("Playstyle.bin", "compact_legacy_records"), ("PlayerSkill.bin", "compact_legacy_records")],
        "fix3": [("Playstyle.bin", "compact_legacy_records"), ("PlayerSkill.bin", "compact_legacy_records"),
                 ("Player.bin", "ios_national_card_flag"), ("PlayerSkill.bin", "ios_skill_numbering"),
                 ("PlayerVariationPrSkill.bin", "ios_skill_numbering")],
    }

    def test_every_shipped_build_is_recorded(self):
        self.assertEqual(SHIPPED, {KONAMI_DT870: {"build138-fix2": FIX2_DT870, "fix3": FIX3_DT870}})

    def test_shipped_entries_keep_their_steps(self):
        names = [name for name, _ in PIPELINES]
        self.assertEqual(names[:len(self.FROZEN)], list(self.FROZEN), "shipped entries stay first, in order")
        for name, steps in dict(PIPELINES).items():
            if name in self.FROZEN:
                self.assertEqual([(s.table, s.step) for s in steps], self.FROZEN[name], name)
        fix3 = dict(PIPELINES)["fix3"]
        self.assertEqual([s.function for s in fix3[2:]], [ios_national_card_flag, skills.renumber_player_skills,
                                                          skills.renumber_featured_skills])


class PlayerSkillTableTest(unittest.TestCase):
    """PlayerSkill.bin: skill priorities, read by the in-match substitution screen."""

    def check(self, raw):
        out = skills.convert_player_skill_table(raw)
        self.assertEqual(len(out), 74 * 4)
        words = struct.unpack("<74I", out)
        self.assertEqual([w & 0x7F for w in words], list(range(74)))
        for a in range(72):
            self.assertEqual((words[skills.android_to_ios_skill(a)] >> 7) & 31, (a * 7) % 29)
        for k, w in skills.IOS_ONLY_PLAYER_SKILL_WORDS.items():
            self.assertEqual(words[k], w)
        return out

    def test_legacy_104_byte_records(self):
        self.check(android_player_skill(104))

    def test_compacted_4_byte_records(self):
        self.assertEqual(self.check(android_player_skill(4)), self.check(android_player_skill(104)))

    def test_already_ios_is_left_alone(self):
        out, info = skills.renumber_player_skills(compact_table(android_player_skill(104)))
        self.assertEqual(info, {"before": 288, "after": 296, "records": 74})
        self.assertEqual(skills.player_skill_numbering(out), "ios")
        self.assertEqual(skills.renumber_player_skills(out), (out, {}))
        self.assertEqual(compact_table(out), out)  # the compaction step leaves the iOS table alone too
        with self.assertRaises(ValueError):
            skills.convert_player_skill_table(out)

    def test_bad_inputs(self):
        with self.assertRaises(ValueError):
            skills.convert_player_skill_table(b"\0" * 100)
        shuffled = bytearray(android_player_skill(4))
        shuffled[0] = 5
        with self.assertRaises(ValueError):
            skills.convert_player_skill_table(bytes(shuffled))

    def test_substitution_screen_ranks_the_same_skills(self):
        # Android's table ranks a card's Android skills; the converted table ranks the same skills under their iOS ids.
        android = compact_table(android_player_skill(104))
        ios = skills.convert_player_skill_table(android)
        for card in ({0, 24, 25, 30, 66, 67, 71}, {3, 13, 26, 60}, {25, 67}):
            before = client_substitution_specials(card, android)
            self.assertEqual([skills.android_to_ios_skill(a) for a in before],
                             client_substitution_specials({skills.android_to_ios_skill(a) for a in card}, ios))

    def test_substitution_model_orders_by_priority(self):
        table = struct.pack("<4I", 0 | 3 << 7, 1 | 0 << 7, 2 | 9 << 7, 3 | 5 << 7)
        self.assertEqual(client_substitution_specials({0, 1, 2, 3}, table), [2, 3, 0])
        self.assertEqual(client_substitution_specials({0, 1, 2, 3}, table, limit=2), [2, 3])
        self.assertEqual(client_substitution_specials({1, 70}, table), [])  # unknown id -> priority 0


@unittest.skipUnless(os.environ.get("DT870_ORIGINAL") and os.environ.get("DT200_ORIGINAL"),
                     "set DT870_ORIGINAL to Konami's dt870_mobile_and.cpk and DT200_ORIGINAL to the data pack's "
                     "cpk/dt200_mobile_all.cpk (Konami's iOS tables)")
class KonamiSkillTablesTest(unittest.TestCase):
    """The converted Android tables equal Konami's own iOS dt200 tables."""

    @classmethod
    def setUpClass(cls):
        cls.android = Path(os.environ["DT870_ORIGINAL"]).read_bytes()
        cls.ios = Path(os.environ["DT200_ORIGINAL"]).read_bytes()
        cls.fixed = repair_cpk_bytes(cls.android)[0]

    def test_player_skill_equals_ios_dt200(self):
        android = decoded_table(self.android, "PlayerSkill.bin")
        ios = decoded_table(self.ios, "PlayerSkill.bin")
        self.assertEqual(skills.convert_player_skill_table(android), ios)
        self.assertEqual(skills.convert_player_skill_table(compact_table(android)), ios)
        self.assertEqual(decoded_table(self.fixed, "PlayerSkill.bin"), ios)

    def test_pr_skill_equals_ios_dt200_on_every_shared_record(self):
        android = decoded_table(self.android, "PlayerVariationPrSkill.bin")
        ios = decoded_table(self.ios, "PlayerVariationPrSkill.bin")
        conv = decoded_table(self.fixed, "PlayerVariationPrSkill.bin")
        self.assertEqual(conv, skills.convert_pr_skill_table(android))
        a = {conv[i:i + 8]: conv[i:i + 16] for i in range(0, len(conv), 16)}
        b = {ios[i:i + 8]: ios[i:i + 16] for i in range(0, len(ios), 16)}
        shared = set(a) & set(b)
        self.assertEqual(len(b), 287)
        self.assertEqual(len(shared), len(b))
        self.assertEqual(sum(a[k] == b[k] for k in shared), len(b))
        self.assertEqual(sum(android[i:i + 16] == b.get(android[i:i + 8]) for i in range(0, len(android), 16)), 0)


class CpkRepairTest(unittest.TestCase):
    def setUp(self):
        rng = random.Random(11)
        styles = b"".join(struct.pack("<I", i | 256) + b"Style".ljust(164, b"\0") for i in range(36))
        skills_table = b"".join(struct.pack("<I", i | 1024) + b"Skill".ljust(100, b"\0") for i in range(72))
        players = bytearray(rng.randrange(256) for _ in range(400 * 60))  # does not compress: grows on repack
        for i in range(60):
            set_bit(players, i * 400 * 8 + 618, i % 3 == 0)
            set_bit(players, i * 400 * 8 + 626, i % 5 == 0)
        self.players = bytes(players)
        self.skills = skills_table
        pick = random.Random(13)
        featured = []
        for i in range(300):  # Android-numbered, mostly empty (72) like Konami's
            slots = [pick.randrange(72) if i % 4 == 0 or (i % 4 == 1 and k == 0) else 72 for k in range(3)]
            featured.append(pr_row(88045755000000 + 37 * i, *slots))
        self.featured = b"".join(featured)
        self.files = [
            ("common/demo", "a.bin", bytes(rng.randrange(256) for _ in range(3000))),
            ("common/etc/pesdb", "Player.bin", konami_wesys(self.players)),
            ("common/etc/pesdb", "PlayerSkill.bin", konami_wesys(skills_table)),
            ("common/etc/pesdb", "PlayerVariationPrSkill.bin", konami_wesys(self.featured)),
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

    def test_default_pipeline_repairs_all_four_tables(self):
        fixed, changes = repair_cpk_bytes(self.cpk)
        report = verify_cpk(self.cpk, fixed)
        self.assertEqual(sorted(report["changed_files"]),
                         ["Player.bin", "PlayerSkill.bin", "PlayerVariationPrSkill.bin", "Playstyle.bin"])
        self.assertEqual(unpack_wesys(self.payload(fixed, "Player.bin")), ios_national_card_flag(self.players)[0])
        self.assertEqual(unpack_wesys(self.payload(fixed, "PlayerSkill.bin")),
                         skills.convert_player_skill_table(compact_table(self.skills)))
        self.assertEqual(unpack_wesys(self.payload(fixed, "PlayerVariationPrSkill.bin")),
                         skills.convert_pr_skill_table(self.featured))
        self.assertEqual([(c["table"], c["step"]) for c in changes if c["step"] != "cpk_layout"],
                         [("Playstyle.bin", "compact_legacy_records"), ("PlayerSkill.bin", "compact_legacy_records"),
                          ("Player.bin", "ios_national_card_flag"), ("PlayerSkill.bin", "ios_skill_numbering"),
                          ("PlayerVariationPrSkill.bin", "ios_skill_numbering")])
        flag = [c for c in changes if c["step"] == "ios_national_card_flag"][0]
        self.assertEqual((flag["bit626_set"], flag["bit626_cleared"]), (16, 8))
        self.assertEqual(repair_cpk_bytes(fixed)[0], fixed)  # running it again changes nothing

    def test_grown_tables_move_later_files_by_whole_blocks(self):
        # As in Konami's dt870, two tables grow: Player.bin and, further on, PlayerVariationPrSkill.bin.
        rng = random.Random(5)
        extra = bytes(rng.randrange(256) for _ in range(9000))  # incompressible: the table must grow
        more = bytes(rng.randrange(256) for _ in range(16 * 300))
        steps = PIPELINES[-1][1] + (TableStep("Player.bin", "test_grow", lambda raw: (raw + extra, {"added": 9000})),
                                    TableStep("PlayerVariationPrSkill.bin", "test_grow", lambda raw: (raw + more, {})))
        fixed, changes = repair_cpk_bytes(self.cpk, steps)
        report = verify_cpk(self.cpk, fixed, steps)
        self.assertEqual(sorted(report["changed_files"]),
                         ["Player.bin", "PlayerSkill.bin", "PlayerVariationPrSkill.bin", "Playstyle.bin"])
        layout = [c for c in changes if c["step"] == "cpk_layout"]
        self.assertEqual([c["table"] for c in layout], ["Player.bin", "PlayerVariationPrSkill.bin"])
        self.assertGreater(len(fixed), len(self.cpk))
        # Every file decodes; untouched files and the filler after them are byte-identical.
        expected_players = ios_national_card_flag(self.players)[0] + extra
        self.assertEqual(unpack_wesys(self.payload(fixed, "Player.bin")), expected_players)
        self.assertEqual(unpack_wesys(self.payload(fixed, "PlayerVariationPrSkill.bin")),
                         skills.convert_pr_skill_table(self.featured) + more)
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
        # Files after Player.bin move by its growth, files after PlayerVariationPrSkill.bin by both, in whole blocks.
        moved = {n["FileName"]: n["FileOffset"] - o["FileOffset"] for o, n in zip(old_rows, new_rows)}
        first, total = moved["PlayerSkill.bin"], len(fixed) - len(self.cpk)
        self.assertEqual((moved["a.bin"], moved["Player.bin"], moved["PlayerVariationPrSkill.bin"]), (0, 0, first))
        self.assertEqual((moved["Playstyle.bin"], moved["z.str"]), (total, total))
        self.assertTrue(0 < first < total and first % 2048 == 0 and total % 2048 == 0)

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
        # The compaction-only pipeline reproduces the copy shipped from build 138 to fix2, the fix3 one the fix3 copy.
        self.assertIn(FIX2_DT870, entry["replaces_sha256"])
        self.assertEqual((entry["pipeline"], entry["sha256"], sha(fixed)), ("fix3", FIX3_DT870, FIX3_DT870))
        flag = [c for c in entry["changes"] if c["step"] == "ios_national_card_flag"][0]
        self.assertEqual((flag["records"], flag["bit626_set"], flag["bit626_cleared"]), (44028, 4078, 44))
        numbering = {c["table"]: c for c in entry["changes"] if c["step"] == "ios_skill_numbering"}
        self.assertEqual(numbering["PlayerSkill.bin"], {"table": "PlayerSkill.bin", "step": "ios_skill_numbering",
                                                        "before": 288, "after": 296, "records": 74})
        featured = numbering["PlayerVariationPrSkill.bin"]
        self.assertEqual([featured[k] for k in ("records", "skills_renumbered", "skills_kept", "empty_slots")],
                         [13147, 1236, 635, 37570])


# ---------------------------------------------------------------- synthetic CRI CPK writer (test only)

def sha(b):
    return hashlib.sha256(b).hexdigest()


def pr_row(card, s0, s1, s2, high=0, tail=0):
    """One PlayerVariationPrSkill.bin record: card id, three 7-bit skill slots (and the word's high bits), zero."""
    return struct.pack("<QII", card, s0 | s1 << 7 | s2 << 14 | high << 21, tail)


def android_player_skill(stride, prio=lambda a: (a * 7) % 29):
    """An Android PlayerSkill.bin: 72 records numbered 0..71 with a priority each, `stride` bytes per record."""
    return b"".join(struct.pack("<I", a | prio(a) << 7) + b"\0" * (stride - 4) for a in range(72))


def decoded_table(cpk, name):
    _, _, rows, _, base = read_cpk(cpk)
    row = [r for r in rows if r.get("DirName") == PESDB_DIR and r.get("FileName") == name][0]
    return unpack_wesys(cpk[base + row["FileOffset"]:base + row["FileOffset"] + row["FileSize"]])


# What the iOS 11.1.1 client does with the two skill tables, for the tests.

def client_featured_skills(word):
    """UtilityMyClub::GetPRCardSkillList (0x1048958c0) for a PlayerVariationPrSkill word: the import swaps
    the 2nd and 3rd slot (LinkPesdb::ImportPlayerVariationPrSkill 0x104ba0f60..0x104ba0f84), and every
    id of 74 or more is left out."""
    s0, s1, s2 = skills.pr_slots(word)
    return [v for v in (s0, s2, s1) if v < skills.IOS_SKILL_COUNT]


def client_substitution_specials(card_skills, player_skill_table, limit=3):
    """The substitution screen (0x10092fd30, with GetSkillCardPriority 0x104bc27e4): the card's skills whose
    PlayerSkill priority is above 0, highest first, the first `limit` of them (its Super-sub case left out)."""
    priority = {}
    for i in range(0, len(player_skill_table), 4):
        w = struct.unpack_from("<I", player_skill_table, i)[0]
        priority.setdefault(w & 0x7F, (w >> 7) & 0x1F)  # GetPlayerSkillData: the first record with that id
    ranked = sorted((k for k in sorted(card_skills) if priority.get(k, 0)), key=lambda k: -priority[k])
    return ranked[:limit]  # equal priorities keep the id order


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
