#!/usr/bin/env python3
"""Make and check ZIRU_PACKS_CONFIG.json, the file every box the game shows is made from.

The server builds every box list the client gets (the box list, the main menu's box list and prices, the box list in
a draw reply) from ZIRU_PACKS_CONFIG.json, and draws each box's cards from it. The format is described in
ziru-extra/README.md ("ZIRU_PACKS_CONFIG.json: the boxes").

    python3 tools/box_layout.py convert LAYOUT.json [--responses DIR] [--out FILE]
        Turns a layout (see below) into a version 2 config, checks it, and writes it to FILE (default
        DIR/ZIRU_PACKS_CONFIG.json). Nothing is written when the check finds an error.
    python3 tools/box_layout.py check [--responses DIR] [--config FILE]
        Checks a config (default DIR/ZIRU_PACKS_CONFIG.json) against the player records and the shipped box list in
        DIR, and prints one line per box. Exit status 1 when it finds an error.

DIR defaults to the repository's ziru-extra/responses.

A layout lists the boxes in the order the game shows them:

    {"boxes": [{"agent_id": 7001, "title": "ZIRU BIG TIME", "category": "bigtime",
                "player_ids": [...], "pickups": [...], "banner": 1348}, ...]}

- agent_id, title, category and player_ids are required; pickups (the featured cards) and banner are optional.
- banner is the agent id of a box in the shipped box list (DIR/CmdGetMyclubAgentlist.json) whose banner pictures and
  text colours the new box reuses, or {"c": <banner_file_c_name>, "d": <banner_file_d_name>} (the keys may also be
  banner_file_c_name and banner_file_d_name, with text_color_a and text_color_b if wanted). Without it, a box looks
  like the shipped box of its category's look (LOOKS below).
- A box may also give "subcategory" (the label shown with it; default LABELS below) and "order" (default: its place
  in the layout, counted from 1).
"""
import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RESPONSES = ROOT / "ziru-extra" / "responses"
SCHEMA_V1 = "ziru-runtime-pack-config-v1"  # Ziru's own file: cards may be in two boxes, no order, no featured cards
SCHEMA_V2 = "ziru-runtime-pack-config-v2"

# Agent ids below this fit the 32-bit numbers older builds of the server compared them as.
AGENT_ID_LIMIT = 2 ** 31
# Ids other response files already use for something else.
RESERVED_AGENT_IDS = {1, 301, 901, 1343, 1358, 1396, 1397, 9985, 999001}
# The game refuses a draw of more cards than a box shows as left (UCMenuMyClubPaymentManager::PaymentKindAgent
# 0x100e19768..0x100e19790), so a smaller box never offers its 10-card draw.
TEN_DRAW = 10

# The label shown with a box of each category, and which shipped box (by its label) a box of that category looks
# like when the layout gives no banner.
LABELS = {"bigtime": "Big Time", "legendary": "Legendary", "epic": "Epic", "showtime": "Show Time",
          "highlight": "Highlight", "others": "Others", "arttest": "Art test"}
LOOKS = {"bigtime": "Big Time", "legendary": "Epic", "epic": "Epic", "showtime": "Show Time",
         "highlight": "Show Time", "others": "Epic", "arttest": "Epic"}
LOOK_KEYS = ("banner_file_c_name", "banner_file_d_name", "text_color_a", "text_color_b")
COLOUR_KEYS = ("color_r", "color_g", "color_b")


def load(path):
    with open(path, "rb") as f:
        return json.loads(f.read())


def is_int(value):
    return isinstance(value, int) and not isinstance(value, bool)


def shipped_boxes(responses):
    """The entries of the shipped box list (the server's templates), by agent id, in file order."""
    entries = load(Path(responses) / "CmdGetMyclubAgentlist.json").get("agent_list") or []
    return {e["agent_id"]: e for e in entries if isinstance(e, dict) and is_int(e.get("agent_id"))}


def look_of(category, banner, shipped):
    """The banner pictures and text colours of a new box."""
    if isinstance(banner, dict):
        look = {key: banner.get(short, banner.get(key)) for short, key in (("c", "banner_file_c_name"), ("d", "banner_file_d_name"))}
        for key in ("text_color_a", "text_color_b"):
            if key in banner:
                look[key] = banner[key]
        return look
    if banner is not None:
        if banner not in shipped:
            raise ValueError("banner %r is not a box of the shipped box list" % (banner,))
        source = shipped[banner]
    else:
        label = LOOKS.get(category)
        source = next((e for e in shipped.values() if e.get("subcategory") == label), None)
        if source is None:
            source = next(iter(shipped.values()), None)
        if source is None:
            raise ValueError("the shipped box list has no box whose look a new box could reuse")
    return {key: source[key] for key in LOOK_KEYS if key in source}


def convert(layout, responses):
    """A version 2 config made from a layout."""
    shipped = shipped_boxes(responses)
    packs = []
    for place, box in enumerate(layout["boxes"], 1):
        missing = [k for k in ("agent_id", "title", "category", "player_ids") if k not in box]
        if missing:
            raise ValueError("box %d of the layout lacks %s" % (place, ", ".join(missing)))
        pack = {"agent_id": box["agent_id"], "order": box.get("order", place), "title": box["title"],
                "category": box["category"],
                "subcategory": box.get("subcategory", LABELS.get(box["category"], box["category"]))}
        pack.update(look_of(box["category"], box.get("banner"), shipped))
        if box.get("pickups"):
            pack["pickup_ids"] = list(box["pickups"])
        pack["player_ids"] = list(box["player_ids"])
        packs.append(pack)
    return {"schema": SCHEMA_V2, "notes": layout.get("notes", ["Made with tools/box_layout.py convert."]), "packs": packs}


def check(config, records, shipped):
    """(errors, warnings, rows): what is wrong with a config, what deserves a look, and one summary row per box."""
    errors, warnings, rows = [], [], []
    schema = config.get("schema")
    if schema not in (SCHEMA_V1, SCHEMA_V2):
        return ["unknown schema %r" % (schema,)], warnings, rows
    v2 = schema == SCHEMA_V2
    packs = config.get("packs")
    if not isinstance(packs, list) or not packs:
        return ["no boxes (packs)"], warnings, rows
    if not shipped:
        errors.append("the shipped box list (CmdGetMyclubAgentlist.json) has no box entry to make the boxes from")
    agents, titles, orders, box_of = set(), set(), [], {}
    for place, pack in enumerate(packs, 1):
        agent = pack.get("agent_id")
        at = "box %d (%r)" % (place, agent)
        if not is_int(agent) or not 0 < agent < AGENT_ID_LIMIT:
            errors.append("%s: agent_id must be a whole number from 1 to %d" % (at, AGENT_ID_LIMIT - 1))
        elif agent in RESERVED_AGENT_IDS:
            errors.append("%s: agent_id is used by other response files" % at)
        elif agent in agents:
            errors.append("%s: agent_id is used by another box" % at)
        agents.add(agent)
        title = pack.get("title")
        if not isinstance(title, str) or not title.strip():
            errors.append("%s: no title" % at)
        elif v2 and title in titles:
            errors.append("%s: title %r is used by another box" % (at, title))
        titles.add(title)
        cards = pack.get("player_ids")
        if not isinstance(cards, list) or not cards:
            errors.append("%s: no cards (player_ids)" % at)
            cards = []
        bad = [c for c in cards if not is_int(c)]
        if bad:
            errors.append("%s: card ids that are not whole numbers: %r" % (at, bad[:5]))
        cards = [c for c in cards if is_int(c)]
        if len(set(cards)) != len(cards):
            errors.append("%s: a card is listed twice in the box" % at)
        no_record = [c for c in cards if str(c) not in records]
        if no_record:
            errors.append("%s: %d cards have no player record: %r" % (at, len(no_record), no_record[:5]))
        for c in set(cards):
            box_of.setdefault(c, []).append(agent)
        pickups = pack.get("pickup_ids")
        if pickups is not None:
            if not isinstance(pickups, list) or not all(is_int(c) for c in pickups):
                errors.append("%s: pickup_ids must be a list of card ids" % at)
            else:
                outside = [c for c in pickups if c not in set(cards)]
                if outside:
                    errors.append("%s: featured cards that are not cards of the box: %r" % (at, outside))
        for key in ("banner_file_c_name", "banner_file_d_name", "subcategory", "category"):
            if key in pack and (not isinstance(pack[key], str) or not pack[key]):
                errors.append("%s: %s must be a text" % (at, key))
        for key in ("banner_file_c_name", "banner_file_d_name"):
            if isinstance(pack.get(key), str) and not pack[key].startswith("/banner/"):
                errors.append("%s: %s must be a /banner/ path" % (at, key))
        for key in ("text_color_a", "text_color_b"):
            if key in pack:
                colour = pack[key]
                if not (isinstance(colour, dict) and all(is_int(colour.get(k)) and 0 <= colour[k] <= 255 for k in COLOUR_KEYS)):
                    errors.append("%s: %s must give color_r, color_g and color_b from 0 to 255" % (at, key))
        if v2:
            for key in ("order", "category"):
                if key not in pack:
                    errors.append("%s: no %s" % (at, key))
            if "order" in pack and not is_int(pack["order"]):
                errors.append("%s: order must be a whole number" % at)
            elif is_int(pack.get("order")):
                orders.append(pack["order"])
        if 0 < len(cards) < TEN_DRAW:
            warnings.append("%s: only %d cards, so its 10-card draw is never offered" % (at, len(cards)))
        rows.append((agent, pack.get("order", place), len(cards), len(pickups or []), title))
    if v2:
        if len(set(orders)) != len(orders):
            errors.append("two boxes have the same order")
        if orders != sorted(orders):
            errors.append("the boxes are not listed in their order")
        twice = {c: a for c, a in box_of.items() if len(a) > 1}
        if twice:
            sample = sorted(twice.items())[:5]
            errors.append("%d cards are in more than one box, for example %r" % (len(twice), sample))
    return errors, warnings, rows


def report(config, responses):
    records = load(Path(responses) / "ZIRU_PLAYER_RECORDS.json")
    errors, warnings, rows = check(config, records, shipped_boxes(responses))
    for agent, order, size, picks, title in rows:
        print("%8s  order %3s  %4d cards  %d featured  %s" % (agent, order, size, picks, title))
    cards = {c for pack in config.get("packs") or [] for c in pack.get("player_ids") or [] if is_int(c)}
    print("%d boxes, %d places, %d different cards" % (len(rows), sum(r[2] for r in rows), len(cards)))
    for w in warnings:
        print("warning: " + w)
    for e in errors:
        print("error: " + e)
    return not errors


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    sub = parser.add_subparsers(dest="command", required=True)
    c = sub.add_parser("convert", help="make a version 2 config from a layout")
    c.add_argument("layout")
    c.add_argument("--responses", default=str(RESPONSES))
    c.add_argument("--out")
    k = sub.add_parser("check", help="check a config")
    k.add_argument("--responses", default=str(RESPONSES))
    k.add_argument("--config")
    args = parser.parse_args(argv)
    try:
        if args.command == "convert":
            config = convert(load(args.layout), args.responses)
            if not report(config, args.responses):
                print("nothing written")
                return 1
            out = Path(args.out or Path(args.responses) / "ZIRU_PACKS_CONFIG.json")
            out.write_text(json.dumps(config, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
            print("wrote %s" % out)
            return 0
        config = load(args.config or Path(args.responses) / "ZIRU_PACKS_CONFIG.json")
        return 0 if report(config, args.responses) else 1
    except (OSError, KeyError, TypeError, ValueError) as e:
        print("error: %s" % e)
        return 1


if __name__ == "__main__":
    sys.exit(main())
