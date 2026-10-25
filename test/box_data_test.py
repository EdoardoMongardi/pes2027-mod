"""Checks on the box config: tools/box_layout.py (the converter and its check), and the config this
repository holds (ziru-extra/responses/ZIRU_PACKS_CONFIG.json)."""
import contextlib
import io
import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import box_layout  # noqa: E402

EPIC_LOOK = {"subcategory": "Epic", "banner_file_c_name": "/banner/agent/001348/c", "banner_file_d_name": "/banner/agent/001348/d",
             "text_color_a": {"color_r": 255, "color_g": 255, "color_b": 255}, "text_color_b": {"color_r": 51, "color_g": 255, "color_b": 195}}
BIG_TIME_LOOK = {"subcategory": "Big Time", "banner_file_c_name": "/banner/agent/001374/c", "banner_file_d_name": "/banner/agent/001374/d",
                 "text_color_a": {"color_r": 0, "color_g": 213, "color_b": 255}, "text_color_b": {"color_r": 255, "color_g": 255, "color_b": 255}}
SHOW_TIME_LOOK = dict(BIG_TIME_LOOK, subcategory="Show Time", banner_file_c_name="/banner/agent/001375/c",
                      banner_file_d_name="/banner/agent/001375/d")
CARDS = list(range(101, 131))


class BoxLayoutToolTest(unittest.TestCase):
    """The converter makes a version 2 config from a layout; the check finds what would break the boxes."""

    def setUp(self):
        self.dir = tempfile.TemporaryDirectory()
        self.responses = Path(self.dir.name)
        self.records = {str(c): {"gameplayer": {"gameplayer_id": {"id": c, "serial": 0}}} for c in CARDS}
        (self.responses / "ZIRU_PLAYER_RECORDS.json").write_text(json.dumps(self.records))
        shipped = [dict(EPIC_LOOK, agent_id=1368), dict(BIG_TIME_LOOK, agent_id=1348), dict(SHOW_TIME_LOOK, agent_id=1374)]
        (self.responses / "CmdGetMyclubAgentlist.json").write_text(json.dumps({"agent_list": shipped, "agent_price_list": []}))
        self.shipped = box_layout.shipped_boxes(self.responses)

    def tearDown(self):
        self.dir.cleanup()

    def layout(self):
        return {"boxes": [
            {"agent_id": 7001, "title": "ZIRU BIG TIME", "category": "bigtime", "player_ids": CARDS[0:10], "pickups": CARDS[0:3]},
            {"agent_id": 7002, "title": "ZIRU LEGENDARY", "category": "legendary", "player_ids": CARDS[10:20], "banner": 1374},
            {"agent_id": 7003, "title": "ZIRU SHOW TIME: GK", "category": "showtime", "player_ids": CARDS[20:30],
             "banner": {"c": "/banner/agent/009999/c", "d": "/banner/agent/009999/d"}, "subcategory": "Show Time GK"},
        ]}

    def errors(self, config):
        return box_layout.check(config, self.records, self.shipped)[0]

    def test_convert_makes_a_version_2_config(self):
        config = box_layout.convert(self.layout(), self.responses)
        self.assertEqual(config["schema"], box_layout.SCHEMA_V2)
        big, legend, gk = config["packs"]
        self.assertEqual([p["order"] for p in config["packs"]], [1, 2, 3], "the layout's order")
        self.assertEqual((big["subcategory"], big["banner_file_c_name"], big["text_color_a"]),
                         ("Big Time", BIG_TIME_LOOK["banner_file_c_name"], BIG_TIME_LOOK["text_color_a"]),
                         "a box looks like the shipped box of its category")
        self.assertEqual(big["pickup_ids"], CARDS[0:3])
        self.assertEqual((legend["subcategory"], legend["banner_file_d_name"]), ("Legendary", SHOW_TIME_LOOK["banner_file_d_name"]),
                         "banner: the look of a shipped box named by its agent id")
        self.assertEqual((gk["subcategory"], gk["banner_file_c_name"], gk["banner_file_d_name"]),
                         ("Show Time GK", "/banner/agent/009999/c", "/banner/agent/009999/d"), "banner: two given paths")
        self.assertNotIn("pickup_ids", legend)
        self.assertEqual(self.errors(config), [])

    def test_banner_paths_may_use_the_configs_key_names(self):
        layout = {"boxes": [{"agent_id": 7004, "title": "ZIRU OTHERS", "category": "others", "player_ids": CARDS[0:10],
                             "banner": {"banner_file_c_name": "/banner/agent/009998/c", "banner_file_d_name": "/banner/agent/009998/d",
                                        "text_color_b": {"color_r": 1, "color_g": 2, "color_b": 3}}}]}
        others = box_layout.convert(layout, self.responses)["packs"][0]
        self.assertEqual((others["subcategory"], others["banner_file_c_name"], others["banner_file_d_name"], others["text_color_b"]),
                         ("Others", "/banner/agent/009998/c", "/banner/agent/009998/d", {"color_r": 1, "color_g": 2, "color_b": 3}))

    def test_convert_writes_only_a_config_that_passes_the_check(self):
        out = self.responses / "ZIRU_PACKS_CONFIG.json"
        layout_path = self.responses / "layout.json"
        bad = self.layout()
        bad["boxes"][1]["player_ids"] = CARDS[5:15]   # five cards in two boxes
        layout_path.write_text(json.dumps(bad))
        with contextlib.redirect_stdout(io.StringIO()) as printed:
            status = box_layout.main(["convert", str(layout_path), "--responses", str(self.responses)])
        self.assertEqual(status, 1)
        self.assertFalse(out.exists())
        self.assertIn("5 cards are in more than one box", printed.getvalue())
        layout_path.write_text(json.dumps(self.layout()))
        with contextlib.redirect_stdout(io.StringIO()):
            status = box_layout.main(["convert", str(layout_path), "--responses", str(self.responses)])
        self.assertEqual(status, 0)
        self.assertEqual(json.loads(out.read_text()), box_layout.convert(self.layout(), self.responses))
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(box_layout.main(["check", "--responses", str(self.responses)]), 0)

    def test_check_finds_each_problem(self):
        good = box_layout.convert(self.layout(), self.responses)

        def broken(change):
            config = json.loads(json.dumps(good))
            change(config["packs"])
            return self.errors(config)

        cases = {
            "agent id used twice": lambda p: p[1].update(agent_id=7001),
            "reserved agent id": lambda p: p[1].update(agent_id=1343),
            "agent id of 2^31": lambda p: p[1].update(agent_id=2 ** 31),
            "agent id 0": lambda p: p[1].update(agent_id=0),
            "box without cards": lambda p: p[1].update(player_ids=[]),
            "card without record": lambda p: p[1]["player_ids"].append(999),
            "card twice in a box": lambda p: p[1]["player_ids"].append(CARDS[10]),
            "card in two boxes": lambda p: p[1]["player_ids"].append(CARDS[0]),
            "featured card of another box": lambda p: p[1].update(pickup_ids=[CARDS[0]]),
            "two boxes with one title": lambda p: p[1].update(title="ZIRU BIG TIME"),
            "two boxes with one order": lambda p: p[1].update(order=1),
            "boxes not in their order": lambda p: p[1].update(order=9),
            "no order": lambda p: p[1].pop("order"),
            "no category": lambda p: p[1].pop("category"),
            "banner that is no /banner/ path": lambda p: p[1].update(banner_file_c_name="agent/1.png"),
            "colour out of range": lambda p: p[1].update(text_color_a={"color_r": 300, "color_g": 0, "color_b": 0}),
            "no title": lambda p: p[1].update(title=""),
        }
        for what, change in cases.items():
            self.assertNotEqual(broken(change), [], what)

    def test_version_1_allows_a_card_in_two_boxes_and_warns_about_small_boxes(self):
        v1 = {"schema": box_layout.SCHEMA_V1, "packs": [
            {"agent_id": 1368, "title": "EPIC", "category": "epic", "player_ids": CARDS[0:12]},
            {"agent_id": 1348, "title": "BIG TIME", "category": "bigtime", "player_ids": CARDS[10:15]}]}
        errors, warnings, rows = box_layout.check(v1, self.records, self.shipped)
        self.assertEqual(errors, [])
        self.assertEqual(len(warnings), 1)
        self.assertIn("only 5 cards", warnings[0])
        self.assertEqual([r[0] for r in rows], [1368, 1348])
        self.assertNotEqual(self.errors({"schema": "something-else", "packs": v1["packs"]}), [])


RESPONSES = ROOT / "ziru-extra" / "responses"


class ShippedBoxesTest(unittest.TestCase):
    """The box config in this repository (ziru-extra/responses), whatever layout it holds. gameplaytest
    (shippedBoxesTest) serves it and checks that every box is in every box list with its price and its size."""

    @classmethod
    def setUpClass(cls):
        cls.config = box_layout.load(RESPONSES / "ZIRU_PACKS_CONFIG.json")
        cls.records = box_layout.load(RESPONSES / "ZIRU_PLAYER_RECORDS.json")
        cls.shipped = box_layout.shipped_boxes(RESPONSES)
        cls.packs = cls.config["packs"]

    def test_the_check_finds_no_error(self):
        errors, warnings, rows = box_layout.check(self.config, self.records, self.shipped)
        self.assertEqual(errors, [])
        self.assertEqual(len(rows), len(self.packs))

    def test_no_box_is_empty(self):
        self.assertEqual([p["agent_id"] for p in self.packs if not p.get("player_ids")], [])

    def test_every_box_card_has_a_record(self):
        missing = [(p["agent_id"], c) for p in self.packs for c in p["player_ids"] if str(c) not in self.records]
        self.assertEqual(missing, [])

    def test_agent_ids_are_unique_and_below_2_to_the_31(self):
        ids = [p["agent_id"] for p in self.packs]
        self.assertEqual(len(ids), len(set(ids)))
        self.assertTrue(all(box_layout.is_int(a) and 0 < a < 2 ** 31 for a in ids), ids)
        self.assertFalse(set(ids) & box_layout.RESERVED_AGENT_IDS)

    def test_featured_cards_are_cards_of_their_box(self):
        outside = [(p["agent_id"], c) for p in self.packs for c in p.get("pickup_ids") or [] if c not in set(p["player_ids"])]
        self.assertEqual(outside, [])

    def test_no_card_is_in_two_boxes(self):
        if self.config.get("schema") == box_layout.SCHEMA_V1:
            self.skipTest("version 1 config (Ziru's own layout): its cards may be in two boxes")
        seen, twice = {}, []
        for p in self.packs:
            for c in set(p["player_ids"]):
                if c in seen:
                    twice.append((c, seen[c], p["agent_id"]))
                seen[c] = p["agent_id"]
        self.assertEqual(twice, [])

    def test_the_template_has_a_box_entry_and_a_price_for_1_and_10_cards(self):
        lists = box_layout.load(RESPONSES / "CmdGetMyclubAgentlist.json")
        self.assertTrue(lists.get("agent_list"))
        prices = lists.get("agent_price_list") or []
        self.assertTrue(prices)
        counts = {x.get("multiple_count") for x in prices[0].get("price_list") or []}
        self.assertTrue({1, box_layout.TEN_DRAW} <= counts, counts)


if __name__ == "__main__":
    unittest.main()
