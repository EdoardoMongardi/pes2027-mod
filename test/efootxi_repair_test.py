"""Verification of repairs for main-menu banners, stadium API, events unlock,
My League season reset, and Division 3 promotion.
"""
import copy
import json
from pathlib import Path
import sqlite3
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
TOOLS = Path(r"D:\Efootxi2027-iOS\source\gateway-release\work\tools")
sys.path.insert(0, str(ROOT / "ios/efootxi"))
sys.path.insert(1, str(TOOLS))

import efootxi_ios as bridge
import efootxi_catalogue as catalogue
import my_league
import ai_division
import match_protocol as protocol


class RepairVerificationTests(unittest.TestCase):
    def setUp(self):
        self.db_path = r"D:\Efootxi2027-iOS\source\gateway-release\outputs\efootball\local-club.sqlite3"
        bridge._gateway = type('Gateway', (), {'_root': str(TOOLS.parents[1])})()

    def test_mainmenu_banner_and_prices_populated(self):
        """Banner clicks resolve to real agents 91000 and 91001 with valid prices."""
        resp = {
            "msgid": "CMD_GET_MAINMENU_INFO",
            "result": "NOERR",
            "rqid": 100,
            "banner_list": [
                {"announcement_type": "AGENT", "entity_id": 91000, "banner_file_name": "/efootxi/player.jpg"},
                {"announcement_type": "AGENT", "entity_id": 91000, "banner_file_name": "/efootxi/coach.jpg"}
            ],
            "agent_list": [],
            "agent_price_list": []
        }
        adapted = bridge._adapt_ios_response({"msgid": "CMD_GET_MAINMENU_INFO"}, resp)
        self.assertEqual(adapted["banner_list"][0]["entity_id"], 91000)
        self.assertEqual(adapted["banner_list"][0]["title_text"], "Cầu thủ đặc biệt")
        self.assertEqual(adapted["banner_list"][1]["entity_id"], 91001)
        self.assertEqual(adapted["banner_list"][1]["title_text"], "Huấn luyện viên & Siêu sao")

        agent_ids = [a["agent_id"] for a in adapted["agent_list"]]
        self.assertIn(91000, agent_ids)
        self.assertIn(91001, agent_ids)

        price_ids = [p["agent_id"] for p in adapted["agent_price_list"]]
        self.assertIn(91000, price_ids)
        self.assertIn(91001, price_ids)

        # Check prices format
        p91000 = next(p for p in adapted["agent_price_list"] if p["agent_id"] == 91000)
        self.assertEqual(p91000["price_list"][0]["price"]["coin_price"], 100)
        self.assertEqual(p91000["price_list"][1]["price"]["coin_price"], 900)

    def test_stadium_data_purged_of_android_vulkan(self):
        """CMD_GET_STADIUM_DATA returns empty data_list to prevent Android Vulkan poisoning."""
        resp = {
            "msgid": "CMD_GET_STADIUM_DATA",
            "rqid": 200,
            "data_list": [{"name": "pc3002_mobile_and.pak"}, {"name": "pc3012_mobile_and.pak"}]
        }
        adapted = bridge._adapt_ios_response({"msgid": "CMD_GET_STADIUM_DATA"}, resp)
        self.assertEqual(adapted["data_list"], [])
        self.assertEqual(adapted["result"], "NOERR")

    def test_match_creative_info_events_never_expire(self):
        """Events in match creative info have far-future end times so they are never locked."""
        resp = {
            "msgid": "CMD_GET_MAINMENU_MATCH_CREATIVE_INFO",
            "rqid": 300,
            "event_info_list": [
                {"event_match_label": "DREAM", "event_info_type": "AI_MATCH", "event_end_time": 1792634399},
                {"event_match_label": "DREAM", "event_info_type": "USER_MATCH", "event_end_time": 1792634399}
            ],
            "ml_event_info": {"event_id": 100004, "event_end_time": 1792634399, "event_panel_close_time": 1792634399},
            "league_info": {"league_phase_end_time": 1792634399}
        }
        adapted = bridge._adapt_ios_response({"msgid": "CMD_GET_MAINMENU_MATCH_CREATIVE_INFO"}, resp)
        for ev in adapted["event_info_list"]:
            self.assertEqual(ev["event_end_time"], 2147483647)
        self.assertEqual(adapted["ml_event_info"]["event_end_time"], 2147483647)
        self.assertEqual(adapted["league_info"]["league_phase_end_time"], 2147483647)

    def test_event_list_and_log_has_vietnamese_titles(self):
        """CMD_GET_EVENT_LIST_AND_LOG serves all 5 unlocked Vietnamese events."""
        resp = {"msgid": "CMD_GET_EVENT_LIST_AND_LOG", "rqid": 400, "event_list": []}
        adapted = bridge._adapt_ios_response({"msgid": "CMD_GET_EVENT_LIST_AND_LOG"}, resp)
        self.assertEqual(len(adapted["event_list"]), 5)
        names = [ev["event_name"] for ev in adapted["event_list"]]
        self.assertIn("UEFA Champions League - Cúp C1 Châu Âu", names)
        self.assertIn("FIFA World Cup - Cúp Vô Địch Thế Giới", names)
        self.assertIn("Legend All-Stars Challenge - Thách Đấu Huyền Thoại", names)
        self.assertIn("European Super Derbies - Siêu Kinh Điển El Clásico", names)
        self.assertIn("My League Sprint - Đua Tốc Độ & Thử Thách", names)
        for ev in adapted["event_list"]:
            self.assertEqual(ev["event_end_time"], 2147483647)
        self.assertIn("event_log_list", adapted)
        self.assertEqual(len(adapted["event_log_list"]), 5)
        for log in adapted["event_log_list"]:
            self.assertEqual(log["entry_status"], "ENTRY")

    def test_my_league_season_resets_after_completion(self):
        """When My League season is completed (fixture > 38), re-entry resets to next season fixture 1."""
        c = sqlite3.connect(':memory:')
        c.executescript('''CREATE TABLE meta(key TEXT PRIMARY KEY, value TEXT);
            CREATE TABLE squads(squad_id INTEGER PRIMARY KEY, json TEXT);
            CREATE TABLE match_receipts(mode TEXT, rqid TEXT, response TEXT, PRIMARY KEY(mode, rqid));''')
        c.execute("INSERT INTO squads VALUES(1, ?)", (json.dumps({"squad_id": 1, "squad_data": {}}),))
        # Setup finished season
        finished_season = {
            "team_id": 100,
            "competition_id": 113,
            "fixture": 39,
            "played": {str(i): {"result": "WIN", "matches": []} for i in range(1, 39)},
            "completed": True,
            "active": None,
            "season": 1
        }
        c.execute("INSERT INTO meta VALUES('my_league_state_v1', ?)", (json.dumps(finished_season),))
        c.commit()

        # Call select with resume request (select_team_id = 0)
        req = {"msgid": "CMD_SET_SEASON_ENTER", "rqid": 500, "select_team_id": 0}
        resp = my_league.select(c, {}, req)
        self.assertEqual(resp["result"], "NOERR")
        self.assertEqual(resp["team_id"], 100)

        # Call info to check season view
        info_resp = my_league.info(c, {}, {"msgid": "CMD_GET_SEASON_INFO", "rqid": 501})
        self.assertEqual(info_resp["fixture"], 1)
        self.assertEqual(info_resp["is_end_season"], "NO")

        # Verify state in DB has advanced season and reset fixture
        saved = json.loads(c.execute("SELECT value FROM meta WHERE key='my_league_state_v1'").fetchone()[0])
        self.assertEqual(saved["season"], 2)
        self.assertEqual(saved["fixture"], 1)
        self.assertFalse(saved["completed"])
        self.assertEqual(len(saved["played"]), 0)

    def test_division_3_promotes_with_three_wins(self):
        """Division 3 promotes when reaching 3 wins or 1400 points."""
        # Test 3 wins promotion
        s = {
            "division": 3,
            "best": 3,
            "points": 1390,
            "matches": 3,
            "wins": 3,
            "draws": 0,
            "losses": 0,
            "round": 1,
            "point_mode": "RATING"
        }
        outcome = ai_division._transition(s)
        self.assertEqual(outcome, (2, "PROMOTED"))

        # Test threshold promotion (1400 points)
        s2 = {
            "division": 3,
            "best": 3,
            "points": 1400,
            "matches": 4,
            "wins": 2,
            "draws": 2,
            "losses": 0,
            "round": 1,
            "point_mode": "RATING"
        }
        outcome2 = ai_division._transition(s2)
        self.assertEqual(outcome2, (2, "PROMOTED"))

    def test_copyright_notice_popup_and_dismissal(self):
        """One-time startup notice appears once with exact copyright text and dismisses permanently on OK."""
        # Check notice content in notices json
        import announcements
        records = announcements.records()
        self.assertTrue(len(records) > 0)
        notice_140001 = next((n for n in records if n["newsId"] == 140001), None)
        self.assertIsNotNone(notice_140001)
        self.assertEqual(notice_140001["frequency_type"], "ONCE_POPUP_ONLY")
        self.assertIn("Bản quyền thuộc về Efootxi và iOSVN chỉ là người biên dịch", notice_140001["body"])

        # Test in-memory DB for read acknowledgment
        c = sqlite3.connect(':memory:')
        c.executescript('''CREATE TABLE meta(key TEXT PRIMARY KEY, value TEXT);''')

        # Initially unread -> unread_popup is True -> is_news_popup should be YES
        resp = {"msgid": "CMD_GET_MAINMENU_INFO", "rqid": 600}
        app_resp = announcements.apply(c, "CMD_GET_MAINMENU_INFO", {"msgid": "CMD_GET_MAINMENU_INFO"}, resp)
        self.assertEqual(app_resp["is_news_popup"], "YES")

        # Native news returns ONCE_POPUP_ONLY frequency
        mail_resp = announcements.apply(c, "CMD_GET_MAILBOX_INFO", {"msgid": "CMD_GET_MAILBOX_INFO"}, {"msgid": "CMD_GET_MAILBOX_INFO"})
        mail_notice = next(n for n in mail_resp["news_list"] if n["news_id"] == 140001)
        self.assertEqual(mail_notice["frequency_type"], "ONCE_POPUP_ONLY")
        self.assertEqual(mail_notice["readFlag"], "NO")

        # Player taps OK / acknowledges news 140001
        ack_req = {"msgid": "CMD_SET_NEWSLIST", "news_list": [140001], "rqid": 601}
        ack_resp = announcements.apply(c, "CMD_SET_NEWSLIST", ack_req, {})
        self.assertEqual(ack_resp["result"], "NOERR")

        # Next main menu call: is_news_popup MUST be NO
        resp2 = {"msgid": "CMD_GET_MAINMENU_INFO", "rqid": 602}
        app_resp2 = announcements.apply(c, "CMD_GET_MAINMENU_INFO", {"msgid": "CMD_GET_MAINMENU_INFO"}, resp2)
        self.assertEqual(app_resp2["is_news_popup"], "NO")

        # And mailbox reflects readFlag = YES
        mail_resp2 = announcements.apply(c, "CMD_GET_MAILBOX_INFO", {"msgid": "CMD_GET_MAILBOX_INFO"}, {"msgid": "CMD_GET_MAILBOX_INFO"})
        mail_notice2 = next(n for n in mail_resp2["news_list"] if n["news_id"] == 140001)
        self.assertEqual(mail_notice2["readFlag"], "YES")


if __name__ == '__main__':
    unittest.main()
