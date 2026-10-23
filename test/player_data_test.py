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

    def test_starter_cards_without_a_record_use_konamis_login_values(self):
        capture = json.loads((RESPONSES / LOGIN_CAPTURE).read_bytes())["entry_info"]["gameplayer_list"]
        konami = {}
        for player in capture:
            konami.setdefault(card_id(player), (player["play_style"], player["parameters"][61:70]))
        bad, checked = [], 0
        for name in sorted(STARTER_FILES):
            squad = json.loads((RESPONSES / name).read_bytes())["entry_info"]["gameplayer_list"]
            for i, player in enumerate(squad):
                cid = card_id(player)
                if str(cid) in self.records:
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


if __name__ == "__main__":
    unittest.main()
