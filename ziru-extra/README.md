# ziru-extra

Server responses written for this port (not taken from Ziru's APK). `build_ipa.py` copies `responses/*` into
`PESmobile.app/ziru/responses/` after Ziru's files, so a file here adds a missing command or overrides Ziru's.

- `CMD_GET_AGE_GATE_REQUIREMENTS.json`: new in client 11.1.1 (Ziru's server targets 11.0.1 and has no response).
  Fields read by `CmdGetAgeGateRequirementsResult::UnPackJson` (0x103b38128): `result`, `is_eula_country`
  (YES/NO, compared with `CommandCamYesno` literal 1), `country_age_kind` (NONE / BIRTH_DATE / BIRTH_YEAR_MONTH,
  `EMenuEulaCountryAgeKind`). Values match Ziru's `get_user_eula_info.json` (`eula_country_age_kind: BIRTH_YEAR_MONTH`).
  With `is_eula_country: YES` QuitCommand keeps the stored birthday; `NO` clears it and reruns the birthday flow.

## `ZIRU_PACKS_CONFIG.json`: the boxes

Every box the game shows comes from this file. The server makes all the box lists the client gets from it (the box
list `CMD_GET_MYCLUB_AGENTLIST`, the box list and prices in the main menu, the box list in a draw reply), and a draw,
the box contents screen, the box info and a reset use the same boxes (`core/srv_myclub.inc`, "The box list the client
gets"). The agent lists in `CmdGetMyclubAgentlist.json`, `get_myclub_agentlist.json` and
`get_myclub_mainmenu_info.json` no longer decide which boxes exist. `CmdGetMyclubAgentlist.json` is the template: each
box's entry and price entry is a copy of its entry there with the same agent id, else of its first entry, with the
config's values put in.

```json
{
  "schema": "ziru-runtime-pack-config-v2",
  "notes": ["free text"],
  "packs": [
    {
      "agent_id": 7001,
      "order": 1,
      "title": "ZIRU BIG TIME",
      "category": "bigtime",
      "subcategory": "Big Time",
      "banner_file_c_name": "/banner/agent/001374/308aa700...",
      "banner_file_d_name": "/banner/agent/001374/8265c6bb...",
      "text_color_a": {"color_r": 0, "color_g": 213, "color_b": 255},
      "text_color_b": {"color_r": 255, "color_g": 255, "color_b": 255},
      "pickup_ids": [89138556572074, 89138556575063, 89138556678270],
      "player_ids": [89138556572074, 89138556575063, 89138556678270, 89138556572075]
    }
  ]
}
```

| Field | | What the server does with it |
|---|---|---|
| `agent_id` | required | The box's id. 1 to 2^31−1, one box per id, and not an id other response files use (1, 301, 901, 1343, 1358, 1396, 1397, 9985, 999001). |
| `order` | required in v2 | Boxes are listed by `order`, all in one scrolling list (the game has no tabs or sorting). The file lists them in that order. |
| `title` | required | The box name shown in the game. |
| `category` | required in v2 | What kind of box it is (`bigtime`, `legendary`, `epic`, `showtime`, `highlight`, `others`, `arttest`). Used by `tools/box_layout.py`, not by the server. |
| `player_ids` | required | The box's cards. A card without a record in `ZIRU_PLAYER_RECORDS.json` is left out (a draw cannot give it). A box with no card left is not listed. The box's size (`player_list_total`, `base_player_count`) is the number of these cards. |
| `pickup_ids` | optional | The featured cards on the box's banner. Only cards of the box are shown. Without `pickup_ids` the box shows the featured cards of its own template entry (same agent id) that are in the box; when none is left either way, its first cards, as many as the template shows. |
| `subcategory` | optional | The label shown with the box (else the template's). |
| `banner_file_c_name`, `banner_file_d_name` | optional | The box's two banner pictures (else the template's). |
| `text_color_a`, `text_color_b` | optional | The colours of the box's texts (else the template's). |

What is left in each box is kept in the save with a fingerprint of the box's cards. When a box is new to the save, or
its cards change, it starts again as its cards minus the cards the user has (saved and drawn cards, and the starter
cards of a save that has them; sold cards do not count), so a save moved to a new layout is offered nothing it already
has. A reset box holds all its cards again.

A new save (one that holds nothing yet) starts with no players: none of the starter cards of `CmdLogin.json`, an empty
squad and an empty first MyLeague squad; the coaches and the formation stay. There the starter cards are offered by the
boxes like any other card. A save that already holds anything keeps its starter cards.

A card drawn from a box keeps its own id: only a starter card, held under the serial `CmdLogin.json` lists it under,
is shown as the Big Time Neymar 89133993205152 when it is the Standard Neymar 40352, or as the Show Time De Bruyne
106788187843931 when it is the Standard De Bruyne 44379. The five cards whose card background the client lacks
(Casemiro, Eriksen, Mkhitaryan, Calhanoglu, Fati) are still shown as the card of that player it has, as before.

Version 1 (`ziru-runtime-pack-config-v1`, Ziru's own file) has `agent_id`, `pack_id`, `title`, `category`,
`base_count` and `player_ids`: boxes are listed in file order, a card may be in two boxes, and the featured cards are
those of the box's template entry that are in the box.

`tools/box_layout.py` turns a layout into a version 2 config and checks a config:

```
python3 tools/box_layout.py convert LAYOUT.json [--responses ziru-extra/responses] [--out FILE]
python3 tools/box_layout.py check [--responses ziru-extra/responses] [--config FILE]
```

A layout is `{"boxes": [{"agent_id", "title", "category", "player_ids", "pickups", "banner"}, ...]}` in the order the
game shows the boxes; a box may also give `subcategory` (default: the category's label, such as "Big Time" or
"Legendary") and `order`. `banner` is the agent id of a box in `CmdGetMyclubAgentlist.json` whose pictures and colours
the box reuses, or `{"c": ..., "d": ...}` (two banner paths); without it the box looks like the shipped box of its
category (Big Time boxes like the Big Time box, Show Time and Highlight boxes like a Show Time box, the others like an
Epic box). The records and the shipped box list are read from `--responses`. The check
fails on: an agent id out of range, reserved or used twice; a box without cards; a card listed twice in a box, or
without a record; a featured card that is not in its box; a bad banner path or colour; and in version 2 also a missing
`order` or `category`, two boxes with one title or one order, boxes not listed in their order, and a card in more than
one box. It warns about a box with fewer than 10 cards, whose 10-card draw the game never offers.

## `ZIRU_PLAYER_RECORDS.json`: starter copies

A box can hand out a card only when the card has a record. The record of a starter card made for the boxes is a copy
of the card's own entry in `CmdLogin.json` (its first entry when it is listed twice), marked in its wrapper:

```json
"17592186051927": {"gameplayer": {"gameplayer_id": {"id": 17592186051927, "serial": 0}, ...}, "is_dummy": "NO", "starter_copy": "YES"}
```

- On a save without the starter cards, the copy is the card a box gives: the drawn card, its training and its match
  data come from the record.
- On a save with the starter cards, the starter card it holds stays exactly as before such records existed: it comes
  from its starter entry and what the save holds for it, and so does a card of that id the client names under a serial
  that does not hold it. A copy that save draws from a box (after a reset) is held under its own serial and is the
  record's card.
- A card's styles, AI styles, club, nation and own skills are read from the starter entry, not from the copy, and a
  build 138 save treats the copy as no record when it is moved, since build 138 had no such records. So a copy keeps
  the entry's card (styles, club, nation, parameters, boosters, rarity, type and level cap) and may differ only in the
  state of Konami's captured account (level, experience, points, games, sell value, serial);
  `test/player_data_test.py` checks it.
- The mark is for the server: no answer the client gets carries it.
