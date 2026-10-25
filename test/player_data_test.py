"""Checks on the player data shipped in ziru-extra/responses.

The game builds two lists from a card's two style slots (parameters 61 and 62):
an attack list and a defence list, by each style's attribute in Playstyle.bin.
In a match a style is used only when its list has exactly one entry, so a card
with two styles on one side plays with neither. The card screen reads
play_style, while the user's players in a match started by CMD_START_GAME use
parameters 61/62, so both must hold the same two values.

The player records (ZIRU_PLAYER_RECORDS.json) are the one source for a card's
styles, AI styles, special skills, club and nation; every copy of a card in
the other response files must agree with its record.
"""
import json
import re
import subprocess
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RESPONSES = ROOT / "ziru-extra" / "responses"
RECORDS = RESPONSES / "ZIRU_PLAYER_RECORDS.json"

# Playstyle.bin attribute of every style value. 0 is Basic and is never listed.
ATTACK = set(range(1, 7)) | set(range(10, 16)) | set(range(18, 23)) | {34, 35}
BOTH = {7, 8}                      # Box-to-Box, Anchor Man: listed on both sides
DEFENCE = {9, 16, 17} | set(range(23, 34))
STYLES = {0} | ATTACK | BOTH | DEFENCE
assert STYLES == set(range(36))

# Copies of the records that the server never reads.
NOT_READ = {"ZIRU_PLAYER_RECORDS_DEFAULT.json", "ZIRU_PLAYER_RECORDS_MAXPING.json"}
# Konami's own login response, kept for reference; the server does not serve it.
LOGIN_CAPTURE = "CmdLoginbackup.json"
# Older entry-info layout: play_style is a single number and there is no parameters list.
OLD_FORMAT_FILES = {"CmdSetMyclubEntryInfo.json", "set_myclub_entry_info.json"}
STARTER_FILES = {"CmdLogin.json", "CmdGetEntryInfo.json", "CmdGetMyclubEntryInfo.json"}
# The mark of a record that is a copy of a starter card's CmdLogin.json entry, made for the boxes (ziru-extra/README.md),
# and the card fields it keeps from that entry.
STARTER_COPY = "starter_copy"
STARTER_COPY_FIELDS = ("play_style", "initial_base_team_id", "initial_base_national_team_id", "initial_booster_id_list",
                       "default_rarity", "type")
# A card's ability values (parameters 14..42). Progression points raise them by training group: each step of a group
# adds 1 to each of its values, a step costs 1 point for steps 1 to 4, 2 for steps 5 to 8 and so on, and a card earns
# 2 points for every level above 1. Jumping (39) is in two groups. Weak Foot Accuracy (33), Form (41) and Injury
# Resistance (42) are in none.
ABILITIES = range(14, 43)
TRAINING_GROUPS = {"Shooting": (20, 27, 28), "Passing": (21, 22), "Dribbling": (17, 18, 19), "Dexterity": (14, 36, 38),
                   "Lower Body Strength": (34, 37, 40), "Aerial Strength": (23, 35, 39), "Defending": (15, 24, 25, 26),
                   "GK 1": (16, 39), "GK 2": (30, 32), "GK 3": (29, 31)}
STANDARD = 0x00   # card type (id bits 40..47) of a Standard card, whose values change with Konami's live updates
SPECIAL_SKILLS = range(114, 142)
DUMMY_SKILLS = {9, 14, 42, 50, 51, 72, 73}


def skill_parameters():
    """Skill id -> parameter index, read from core/ios_compat.hpp."""
    text = (ROOT / "core" / "ios_compat.hpp").read_text()
    body = re.search(r"kZiruSkillParameters\[74\]\s*=\s*\{([^}]*)\}", text).group(1)
    return [int(v) for v in body.replace("\n", " ").split(",") if v.strip()]


def response_files():
    for path in sorted(RESPONSES.rglob("*.json")):
        if path.name in NOT_READ or path == RECORDS:
            continue
        try:
            data = json.loads(path.read_bytes())
        except ValueError:
            continue
        yield path, data


def players(node, where=""):
    """Every object that carries a card's styles: a play_style key or a full parameters list."""
    if isinstance(node, dict):
        params = node.get("parameters")
        if "play_style" in node or (isinstance(params, list) and len(params) >= 70):
            yield where, node
        for key, value in node.items():
            yield from players(value, where + "/" + key)
    elif isinstance(node, list):
        for i, value in enumerate(node):
            yield from players(value, "%s[%d]" % (where, i))


def card_id(player):
    gid = player.get("gameplayer_id")
    return gid.get("id") if isinstance(gid, dict) else None


def step_cost(steps):
    return sum(1 + step // 4 for step in range(steps))


def training_steps(trained, untrained):
    """Steps per training group that turn the values `untrained` into `trained`, or None when no such steps exist."""
    gain = {i: trained[i] - untrained[i] for i in ABILITIES}
    jump = 39
    steps = {}
    for group, values in TRAINING_GROUPS.items():
        own = [gain[i] for i in values if i != jump]
        if len(set(own)) != 1 or own[0] < 0:
            return None
        steps[group] = own[0]
    if gain[jump] != steps["Aerial Strength"] + steps["GK 1"]:
        return None
    if any(gain[i] for i in ABILITIES if not any(i in values for values in TRAINING_GROUPS.values())):
        return None
    return steps


def style_problems(player):
    """What would make the shown styles differ from the ones used in a match."""
    style = player.get("play_style")
    params = player.get("parameters") if isinstance(player.get("parameters"), list) else None
    if style is not None:
        if not (isinstance(style, list) and len(style) == 2 and all(isinstance(v, int) for v in style)):
            return ["play_style %r is not two numbers" % (style,)]
        if params is not None and params[61:63] != style:
            return ["parameters 61/62 %r differ from play_style %r" % (params[61:63], style)]
        pair = style
    elif params is not None:
        pair = params[61:63]
    else:
        return ["no play_style and no parameters"]
    problems = []
    unknown = [v for v in pair if v not in STYLES]
    if unknown:
        problems.append("unknown style %r" % unknown)
    attack = sum(1 for v in pair if v in ATTACK or v in BOTH)
    defence = sum(1 for v in pair if v in DEFENCE or v in BOTH)
    if attack > 1 or defence > 1:
        problems.append("%r has %d attack and %d defence styles" % (pair, attack, defence))
    if params is not None and any(v not in (0, 1) for v in params[63:70]):
        problems.append("AI style flags %r are not 0/1" % (params[63:70],))
    return problems


class PlayerDataTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.records = json.loads(RECORDS.read_bytes())
        cls.files = list(response_files())

    def test_records_have_one_style_per_side(self):
        bad = []
        for key, record in self.records.items():
            player = record["gameplayer"]
            self.assertIn("play_style", player, key)
            bad += ["%s: %s" % (key, p) for p in style_problems(player)]
        self.assertEqual(bad, [])
        self.assertGreater(len(self.records), 1000)

    def test_every_shipped_player_has_one_style_per_side(self):
        bad, checked = [], 0
        for path, data in self.files:
            for where, player in players(data):
                if not isinstance(player.get("play_style"), (list, type(None))) and "parameters" not in player:
                    if path.name not in OLD_FORMAT_FILES:
                        bad.append("%s%s: play_style %r without parameters" % (path.name, where, player["play_style"]))
                    continue
                checked += 1
                bad += ["%s%s: %s" % (path.name, where, p) for p in style_problems(player)]
        self.assertEqual(bad, [])
        self.assertGreater(checked, 900)

    def test_copies_of_a_card_match_its_record(self):
        skill_param = skill_parameters()
        bad, checked = [], 0
        for path, data in self.files:
            if path.name == LOGIN_CAPTURE:
                continue
            for where, player in players(data):
                record = self.records.get(str(card_id(player)))
                params = player.get("parameters")
                if record is None or not isinstance(params, list):
                    continue
                checked += 1
                rec = record["gameplayer"]
                learned = {skill_param[s] for s in player.get("additional_skill_id") or []
                           if isinstance(s, int) and 0 <= s < 72 and s not in DUMMY_SKILLS}
                at = "%s%s (%s)" % (path.name, where, card_id(player))
                if "play_style" in player and player["play_style"] != rec["play_style"]:
                    bad.append("%s play_style %r, record %r" % (at, player["play_style"], rec["play_style"]))
                for key in ("parameters", "original_parameters"):
                    values = player.get(key)
                    if not isinstance(values, list):
                        continue
                    if values[61:70] != rec["parameters"][61:70]:
                        bad.append("%s %s 61..69 %r, record %r" % (at, key, values[61:70], rec["parameters"][61:70]))
                    extra = [i for i in SPECIAL_SKILLS if values[i] != rec["parameters"][i]
                             and not (values[i] == 1 and i in learned)]
                    if extra:
                        bad.append("%s %s special skills differ at %r" % (at, key, extra))
                for key in ("initial_base_team_id", "initial_base_national_team_id"):
                    if key in player and player[key] != rec[key]:
                        bad.append("%s %s %r, record %r" % (at, key, player[key], rec[key]))
        self.assertEqual(bad, [])
        self.assertGreater(checked, 500)

    def test_starter_cards_without_their_own_record_use_konamis_login_values(self):
        # A starter copy is not the starter card's own record: the server shows a save's starter card from its starter
        # entry, so that entry is checked too.
        capture = json.loads((RESPONSES / LOGIN_CAPTURE).read_bytes())["entry_info"]["gameplayer_list"]
        konami = {}
        for player in capture:
            konami.setdefault(card_id(player), (player["play_style"], player["parameters"][61:70]))
        bad, checked = [], 0
        for name in sorted(STARTER_FILES):
            squad = json.loads((RESPONSES / name).read_bytes())["entry_info"]["gameplayer_list"]
            for i, player in enumerate(squad):
                cid = card_id(player)
                if str(cid) in self.records and self.records[str(cid)].get(STARTER_COPY) != "YES":
                    continue
                checked += 1
                want_style, want_params = konami[cid]
                if player["play_style"] != want_style or player["parameters"][61:70] != want_params:
                    bad.append("%s[%d] %s: %r %r, Konami %r %r" % (name, i, cid, player["play_style"],
                                                                     player["parameters"][61:70], want_style, want_params))
                original = player.get("original_parameters")
                if original is not None and original[61:70] != want_params:
                    bad.append("%s[%d] %s: original_parameters 61..69 %r, Konami %r" % (name, i, cid, original[61:70], want_params))
        self.assertEqual(bad, [])
        self.assertGreater(checked, 0)

    def test_starter_copies_are_their_untrained_starter_entry(self):
        # The server reads a starter copy's card (styles, club, nation, own skills) from the starter entry, and a copy
        # drawn from a box gets the record's other values, boosters and level cap: all must be the entry's. Only the
        # state of Konami's captured account (level, experience, points, games, sell value, serial) may differ, and
        # the ability values: the entry is the card of an account that may have spent progression points on it, while
        # a record is the untrained card the server adds a box card's points to. So the entry must be the copy plus
        # exactly the points its owner spent. A Standard card's values change with Konami's live updates, and its
        # entry was captured under older ones, so only the rest of a Standard copy is compared.
        login = json.loads((RESPONSES / "CmdLogin.json").read_bytes())["entry_info"]["gameplayer_list"]
        entries = {}
        for player in login:
            entries.setdefault(card_id(player), player)
        copies = {key: record for key, record in self.records.items() if record.get(STARTER_COPY) == "YES"}
        if not copies:
            self.skipTest("these records have no starter copy")
        bad = []
        for key, record in copies.items():
            copy, entry = record["gameplayer"], entries.get(int(key))
            if entry is None:
                bad.append("%s: a starter copy of a card that is not a starter card" % key)
                continue
            for field in STARTER_COPY_FIELDS:
                if copy.get(field) != entry.get(field):
                    bad.append("%s %s %r, starter entry %r" % (key, field, copy.get(field), entry.get(field)))
            params, own = copy["parameters"], entry["parameters"]
            if [v for i, v in enumerate(params[:len(own)]) if i not in ABILITIES] != [v for i, v in enumerate(own) if i not in ABILITIES]:
                bad.append("%s parameters outside the ability values differ from the starter entry's" % key)
            if (int(key) >> 40) & 0xFF != STANDARD:
                level = entry["level_info"]
                spent = 2 * (level["level"] - 1) - level["customize_point"]
                steps = training_steps(own, params)
                if steps is None:
                    bad.append("%s the starter entry's ability values are not the copy's plus training" % key)
                elif sum(step_cost(n) for n in steps.values()) != spent:
                    bad.append("%s the starter entry is the copy plus %d points of training, its owner spent %d" % (
                        key, sum(step_cost(n) for n in steps.values()), spent))
            if copy["level_info"]["level_limit"] != entry["level_info"]["level_limit"]:
                bad.append("%s level cap %r, starter entry %r" % (key, copy["level_info"]["level_limit"], entry["level_info"]["level_limit"]))
            if copy.get("additional_skill_id") or copy.get("additional_position"):
                bad.append("%s a starter copy carries learned skills or positions" % key)
        self.assertEqual(bad, [])


# Files with Konami's captured squads that refer to the user's starter cards by (id, serial).
TEMPLATE_FILES = ["ZIRU_START_GAME.json", "ZIRU_ML_START_GAME.json", "ZIRU_ML_SET_SEASON_ENTER.json",
                  "ZIRU_ML_GAME_RESULT.json", "ZIRU_GET_VSCOM_GAME_RESULT_TEMPLATE.json"]
BUILD_138 = "75288ee"   # the build whose saves the server moves to the new serials


def card_refs(node, out):
    """(id, serial) of every gameplayer_id under node."""
    if isinstance(node, dict):
        gid = node.get("gameplayer_id")
        if isinstance(gid, dict) and "serial" in gid:
            out.add((gid.get("id"), gid["serial"]))
        for value in node.values():
            card_refs(value, out)
    elif isinstance(node, list):
        for value in node:
            card_refs(value, out)
    return out


def starter_serial_table():
    """Rows (id, before, now, order) of core/starter_serials.inc."""
    text = (ROOT / "core" / "starter_serials.inc").read_text()
    return [tuple(int(v) for v in row) for row in re.findall(r"\{(\d+)LL, (\d+)LL, (\d+)LL, (\d+)\}", text)]


class StarterSerialTest(unittest.TestCase):
    """Every starter card has the serial Konami's own login gives it, one card per serial: the
    server keeps a card's training, learned skills and careerplan points under its serial."""

    @classmethod
    def setUpClass(cls):
        cls.capture = json.loads((RESPONSES / LOGIN_CAPTURE).read_bytes())["entry_info"]
        cls.login = json.loads((RESPONSES / "CmdLogin.json").read_bytes())["entry_info"]

    @staticmethod
    def keys(entry):
        return [(p["gameplayer_id"]["id"], p["gameplayer_id"]["serial"]) for p in entry["gameplayer_list"]]

    @staticmethod
    def squad(entry):
        return [(p["gameplayer_id"]["id"], p["gameplayer_id"]["serial"], p.get("uniform_number"))
                for p in entry["squad_list"][0]["squad_data"]["player_list"]]

    def test_every_starter_card_has_konamis_serial(self):
        want, want_squad = self.keys(self.capture), self.squad(self.capture)
        self.assertEqual(len(want), 70)
        for name in sorted(STARTER_FILES):
            entry = json.loads((RESPONSES / name).read_bytes())["entry_info"]
            self.assertEqual(entry["gameplayer_list"], self.login["gameplayer_list"], name)
            self.assertEqual(entry["squad_list"], self.login["squad_list"], name)
            keys = self.keys(entry)
            self.assertEqual(keys, want, name)
            self.assertEqual(len({s for _, s in keys}), 70, name + ": one card per serial")
            self.assertEqual(self.squad(entry), want_squad, name + ": the default squad is Konami's")
            self.assertTrue(all((i, s) in set(keys) for i, s, _ in want_squad), name + ": every squad card is a listed card")

    def test_starter_serials_are_no_other_cards_serials(self):
        serials = {s for _, s in self.keys(self.login)}
        coaches = {c["coach_id"]["serial"] for c in self.login["coach_list"]}
        self.assertFalse(serials & coaches)
        starters = set(self.keys(self.login))
        others = set()
        for name in TEMPLATE_FILES:
            others |= {r for r in card_refs(json.loads((RESPONSES / name).read_bytes()), set()) if r not in starters}
        self.assertTrue(others)
        self.assertEqual(sorted(r for r in others if r[1] in serials), [], "a template card would be read as one of the user's cards")

    def test_save_move_table_matches_the_data(self):
        table = starter_serial_table()
        self.assertEqual([(i, now) for i, _, now, _ in table], self.keys(self.login))
        squad = {(i, s) for i, s, _ in self.squad(self.capture)}
        groups = {}
        for i, before, now, order in table:
            groups.setdefault((i, before), []).append((order, now))
        for (i, before), copies in groups.items():
            self.assertEqual(sorted(o for o, _ in copies), list(range(len(copies))), (i, before))
            if len(copies) > 1:
                self.assertIn((i, dict(copies)[0]), squad, "order 0 is the copy Konami's squad uses")
        self.assertEqual(len({before for _, before, _, _ in table}), 11)
        try:
            old = subprocess.run(["git", "-C", str(ROOT), "show", BUILD_138 + ":ziru-extra/responses/CmdLogin.json"],
                                 capture_output=True, check=True).stdout
        except (OSError, subprocess.CalledProcessError):
            self.skipTest("build 138 is not in this checkout")
        self.assertEqual([(i, before) for i, before, _, _ in table], self.keys(json.loads(old)["entry_info"]))


if __name__ == "__main__":
    unittest.main()
