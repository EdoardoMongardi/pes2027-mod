// End-to-end encrypted commands, synthetic fixtures (no game assets required).
#include "../core/server.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <sqlite3.h>
#include <sys/wait.h>
using namespace zsv;
static void check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
static void asset(const std::string& root, const char* name, const ObjP& obj) {
  std::ofstream(root + "/responses/" + name) << obj->toString();
}
static ObjP obj(const char* text) { return Object::parse(text); }
static ObjP call(OfflineServer& srv, const char* cmd, ObjP req = newObj()) {
  req->put("msgid", cmd)->put("rqid", 42);
  Request r; r.uri = std::string("/pes22/gate/gate_") + cmd + ".php";
  r.body = zc::encodeRequest(Value(req)); r.headers["content-length"] = std::to_string(r.body.size());
  Response resp = srv.serve(r); Value result;
  check(resp.status == 200 && zc::decodeResponse(resp.body, result) && result.o, "encrypted response");
  check(result.o->optString("result") != "ERROR", result.o->toString().c_str());
  return result.o;
}
static ObjP coachRef(int id, int serial) {
  return newObj()->put("coach_id", newObj()->put("id", id)->put("serial", serial));
}
static int coachId(const ObjP& start) {
  return start->getJSONObject("squad_data_home")->getJSONObject("coach")->getJSONObject("coach_id")->getInt("id");
}
static ObjP result(OfflineServer& s, int64_t id, int a, int b, bool giveup = false) {
  auto req = newObj()->put("game_id", (long long)id)->put("match_result", newObj()->put("game_id", (long long)id)
      ->put("abnormalend_reason", giveup ? "NORMAL_GIVEUP" : "NORMAL"))
      ->put("score", newArr()->put(newObj()->put("score_1st", a))->put(newObj()->put("score_1st", b)));
  if (giveup) req->put("intentional_give_up", "YES");
  return call(s, "CMD_GET_VSCOM_GAME_RESULT", req);
}
int main() {
  try {
    zs::setVerbose(false);
    char temp[] = "/tmp/pes-gameplay-XXXXXX";
    check(mkdtemp(temp), "temporary directory");
    std::string root(temp);
    std::filesystem::create_directories(root + "/responses");
    Config cfg; cfg.assetsDir = root; cfg.dataDir = root + "/save";
    auto p = obj(R"({"gameplayer_id":{"id":1,"serial":101},"level_info":{"level":5,"level_limit":5,"customize_point":0}})");
    auto parameters = newArr(); for (int i = 0; i < 145; ++i) parameters->put(i == 14 ? 70 : 0);
    p->put("parameters", parameters);
    auto squad = newObj()->put("player_list", newArr()->put(newObj()->put("gameplayer_id", p->getJSONObject("gameplayer_id"))))
        ->put("coach", newArr()->put(coachRef(100, 1000)))->put("squad_gameplan", newObj());
    auto entry = newObj()->put("main_squad_id", 9)->put("gameplayer_list", newArr()->put(p))
        ->put("coach_list", newArr()->put(coachRef(100, 1000)->put("possession", 80))->put(coachRef(200, 2000)->put("possession", 90)))
        ->put("squad_list", newArr()->put(newObj()->put("squad_id", 9)->put("squad_data", squad)
        ->put("base_team", newArr()->put(newObj()->put("base_team_id", 108)))));
    asset(root, "CmdLogin.json", newObj()->put("entry_info", entry));
    asset(root, "CmdCreateUser.json", obj(R"({"user_code":"offline","auth_code":"local","session_id":"create"})"));
    asset(root, "ZIRU_PLAYER_RECORDS.json", newObj()->put("1", newObj()->put("gameplayer", p)));
    asset(root, "ZIRU_MATCH_TEAMS.json", obj(R"({"teams":{"108":{"base_team_id":108,"team_name":"Test","team_name_short":"TST","kits":[{"id":1,"shirt_rgb":[0,0,0]},{"id":2,"shirt_rgb":[255,255,255]}]}}})"));
    asset(root, "ZIRU_MATCH_PROFILES.json", obj(R"({"profiles":[{"profile_id":1,"player_ids":[1],"coordinates":[],"team_style":0,"base_team_id":108}]})"));
    asset(root, "ZIRU_GET_VSCOM_OPPONENT.json", obj(R"({"squad_data":{"squad_gameplan":{"formation":[],"match_plan_settings":{}}},"user_info":[{}]})"));
    auto start = obj(R"({"squad_data_home":{"coach":{"coach_id":{"id":999,"serial":999}}},"commentary_info":{}})");
    asset(root, "ZIRU_START_GAME.json", start); asset(root, "ZIRU_ML_START_GAME.json", start);
    asset(root, "ZIRU_GET_GAMERESULT_LIST.json", obj(R"({"game_result_list":[{"player":[{},{}]}]})"));
    asset(root, "ZIRU_GET_VSCOM_GAME_RESULT_TEMPLATE.json", obj(R"({"game_end_result":{"match_result":{"player_data":{}},"extra_match_result":{"league_match_result":{"league_info":{"com_level_info":{},"round_match_result":{}}}}}})"));
    asset(root, "cmd_get_league_com_info.json", obj(R"({"league_info":{"com_level_info":{},"round_match_result":{}}})"));
    asset(root, "generic.json", obj(R"({"result":"NOERR"})"));
    {
      OfflineServer s(cfg);
      check(s.z27State()->getInt("division") == 1, "unlocked D1 by default");
      s.ziruDb->saveMeta("z27_division_state", R"({"division":10,"best":10,"point":0,"matches":0,"win":0,"draw":0,"lose":0,"sequence":0,"completed":{}})");
      s.ziruDb->saveMeta("z27_div1_set", "1");
      auto first = call(s, "CMD_GET_VSCOM_OPPONENT"); auto id = first->getLong("game_id");
      check(coachId(call(s, "CMD_START_GAME", newObj()->put("game_id", (long long)id))) == 100, "selected coach on first start");
      call(s, "CMD_SET_MYCLUB_SQUAD_INFO", newObj()->put("squad_id", 9)->put("squad_data", newObj()->put("coach", newArr()->put(coachRef(200, 2222)))));
      auto retry = call(s, "CMD_START_GAME", newObj()->put("game_id", (long long)id));
      check(coachId(retry) == 200, "coach changes must invalidate same-kit snapshot");
      check(retry->getJSONObject("squad_data_home")->getJSONObject("coach")->getJSONObject("coach_id")->getInt("serial") == 2222, "owned coach serial preserved");
      check(coachId(call(s, "CMD_START_GAME", newObj()->put("game_id", (long long)id))) == 200, "idempotent start");
      s.ziruLastRequestBody = zc::encodeRequest(Value(newObj()->put("rqid", 42)));
      Value ml; check(zc::decodeResponse(s.handleZiruMyLeagueStartGameV71().body, ml) && coachId(ml.o) == 200, "MyLeague default uses owned coach");
      auto special = newObj()->put("squad_data", newObj()->put("coach", newArr()->put(coachRef(100, 3333))));
      s.hydrateZiruMyLeagueCoachV73(special);
      check(special->getJSONObject("squad_data")->getJSONArray("coach")->getJSONObject(0)->getJSONObject("coach_id")->getInt("serial") == 3333, "MyLeague serial preserved");
      auto win1 = result(s, id, 1, 0);
      check(s.z27State()->getInt("point") == 3 && s.z27State()->getInt("division") == 10, "first win");
      result(s, id, 1, 0); check(s.z27State()->getInt("point") == 3, "duplicate result does not count twice");
      id = call(s, "CMD_GET_VSCOM_OPPONENT")->getLong("game_id"); call(s, "CMD_START_GAME");
      auto win2 = result(s, id, 2, 0);
      check(s.z27State()->getInt("division") == 9, "promote as soon as menu target is reached");
      check(win2->getJSONObject("game_end_result")->getJSONObject("extra_match_result")->getJSONObject("league_match_result")->getJSONObject("league_info")->getString("is_promoted") == "YES", "promotion flag");
      auto info = call(s, "CMD_GET_LEAGUE_COM_INFO")->getJSONObject("league_info");
      check(info->getInt("current_division") == 9 && info->getInt("current_point") == 0, "menu reflects committed rank");
      auto login = call(s, "CMD_LOGIN")->getJSONObject("entry_info")->getJSONArray("gameplayer_list")->getJSONObject(0);
      check(login->getJSONObject("level_info")->getInt("customize_point") == 68, "60 bonus plus level points");
      auto trained = Array::parse(parameters->toString()); trained->put(14, 80);
      auto use = newObj()->put("gameplayer_id", p->getJSONObject("gameplayer_id"))->put("parameters", trained)
          ->put("use_list", newArr()->put(newObj()->put("parameter_type", "SHOOT")->put("point", 60)));
      auto spent = call(s, "CMD_USE_MYCLUB_CAREERPLAN", newObj()->put("gameplayer_use_list", newArr()->put(use)))
          ->getJSONArray("careerplan_result_list")->getJSONObject(0)->getJSONObject("gameplayer");
      check(spent->getJSONObject("level_info")->getInt("customize_point") == 8, "bonus can be spent");
      check(spent->getJSONArray("parameters")->getInt(14) == 80, "upgraded stats returned");
    }
    {
      OfflineServer s(cfg);
      check(s.z27State()->getInt("division") == 9, "rank survives process restart");
      auto p2 = call(s, "CMD_LOGIN")->getJSONObject("entry_info")->getJSONArray("gameplayer_list")->getJSONObject(0);
      check(p2->getJSONObject("level_info")->getInt("customize_point") == 8 && p2->getJSONArray("parameters")->getInt(14) == 80, "training and remaining balance survive restart/login");
      call(s, "CMD_LOGIN"); check(s.refreshZiruGameplayerFromCanonical(p2)->getJSONObject("level_info")->getInt("customize_point") == 8, "bonus is not added repeatedly");
      auto id = call(s, "CMD_GET_VSCOM_OPPONENT")->getLong("game_id"); call(s, "CMD_START_GAME");
      result(s, id, 0, 0, true); check(s.z27State()->getInt("point") == 0, "give-up is not a draw point");
      auto reset = newObj()->put("gameplayer_id", p->getJSONObject("gameplayer_id"))->put("use_list", newArr());
      auto p3 = call(s, "CMD_USE_MYCLUB_CAREERPLAN", newObj()->put("gameplayer_use_list", newArr()->put(reset)))
          ->getJSONArray("careerplan_result_list")->getJSONObject(0)->getJSONObject("gameplayer");
      check(p3->getJSONObject("level_info")->getInt("customize_point") == 68 && p3->getJSONArray("parameters")->getInt(14) == 70, "reset refunds correct budget");
      auto useReal = newObj()->put("gameplayer_id", p->getJSONObject("gameplayer_id"))
          ->put("use_list", newArr()->put(newObj()->put("parameter_type", "STRENGTH")->put("point", 12)));
      auto p4 = call(s, "CMD_USE_MYCLUB_CAREERPLAN", newObj()->put("gameplayer_use_list", newArr()->put(useReal)))
          ->getJSONArray("careerplan_result_list")->getJSONObject(0)->getJSONObject("gameplayer");
      check(p4->getJSONObject("level_info")->getInt("customize_point") == 56, "12 points spent");
      check(p4->getJSONArray("parameters")->getInt(37) == 8, "kicking power auto calculated from progression points");
      check(p4->getJSONArray("parameters")->getInt(34) == 8, "speed auto calculated from progression points");
      check(p4->getJSONArray("parameters")->getInt(40) == 8, "stamina auto calculated from progression points");
      auto entryTest = call(s, "CMD_SET_MYCLUB_ENTRY_INFO", newObj()->put("base_team_id", 262143)->put("team_name", "Entry Team"));
      check(entryTest->getJSONArray("base_team")->getJSONObject(0)->getString("base_team_license") == "UNKNOWN", "entry info preserves unknown license instead of fabricating licensed assets");
    }
    // iOS wire layout, resume through CREATE_USER, and stale difficulty migration.
    cfg.iosPlayerLayout = true;
    {
      OfflineServer s(cfg);
      auto saved = Object::parse(*s.loadPlayer(101));
      saved->put("initial_booster_id_list", newArr()->put(12)->put(0)->put(0));
      auto par = saved->getJSONArray("parameters"); par->put(142, 12)->put(143, 77)->put(144, 0);
      s.ziruDb->savePlayer(101, 1, saved->toString());
      auto extra = Object::parse(saved->toString());
      extra->getJSONObject("gameplayer_id")->put("serial", 202);
      s.ziruDb->savePlayer(202, 1, extra->toString());
      auto wire = call(s, "CMD_LOGIN")->getJSONObject("entry_info")->getJSONArray("gameplayer_list")->getJSONObject(0)->getJSONArray("parameters");
      check(wire->length() == 147 && wire->getInt(142) == 0 && wire->getInt(143) == 0 && wire->getInt(144) == 12 && wire->getInt(145) == 77, "iOS booster slots are 144..146, not skill bits 142/143");
      check(Object::parse(*s.loadPlayer(101))->getJSONArray("parameters")->length() == 145, "wire translation must not mutate saved layout");
      auto use = newObj()->put("gameplayer_id", p->getJSONObject("gameplayer_id"))->put("parameters", wire)
          ->put("use_list", newArr()->put(newObj()->put("parameter_type", "STRENGTH")->put("point", 12)));
      auto trained = call(s, "CMD_USE_MYCLUB_CAREERPLAN", newObj()->put("gameplayer_use_list", newArr()->put(use)))
          ->getJSONArray("careerplan_result_list")->getJSONObject(0)->getJSONObject("gameplayer")->getJSONArray("parameters");
      check(trained->length() == 147 && trained->getInt(144) == 12 && trained->getInt(145) == 77, "iOS request roundtrip preserves trained boosters");
      auto base = Object::parse(p->toString());
      auto original = base->getJSONArray("parameters");
      while (original->length() < 149) original->put(0);
      base->put("initial_booster_id_list", newArr()->put(12)->put(0)->put(0));
      asset(root, "ZIRU_PLAYER_RECORDS.json", newObj()->put("1", newObj()->put("gameplayer", base)));
      s.ziruPlayerRecordsCache.reset();
      auto pd = call(s, "CMD_GET_MYCLUB_PLAYER_DATA", newObj()->put("gameplayer_id", p->getJSONObject("gameplayer_id")))->getJSONArray("original_parameters");
      check(pd->length() == 147 && pd->getInt(144) == 12 && pd->getInt(145) == 77, "149-element canonical records preserve owned booster slots");
      s.ziruDb->saveMeta("z27_division_state", R"({"division":10,"best":10,"point":0,"matches":0,"win":0,"draw":0,"lose":0,"sequence":0,"completed":{}})");
      s.ziruDb->saveMeta("z27_div1_set", "");
      auto active = s.z27Meta("z27_active_match");
      active->put("division", 10)->put("cpu_level", "EASY");
      s.ziruDb->saveMeta("z27_active_match", active->toString());
      check(sqlite3_exec(s.ziruDb->db, "DELETE FROM meta WHERE k='z27_div1_set'", nullptr, nullptr, nullptr) == SQLITE_OK, "prepare legacy save");
      check(s.z27State()->getInt("division") == 1, "legacy rank migrates to D1");
      auto selected = call(s, "CMD_GET_VSCOM_OPPONENT");
      check(selected->getString("cpu_level") == "LEGEND" && s.z27Active()->getInt("division") == 1, "cached EASY opponent migrates with D1");
      sqlite3_exec(s.ziruDb->db, "PRAGMA query_only=ON", nullptr, nullptr, nullptr);
      bool failed = false;
      try { s.ziruDb->saveMeta("must_not_succeed", "1"); } catch (const std::exception&) { failed = true; }
      check(failed, "write failure must not be acknowledged as saved");
      sqlite3_exec(s.ziruDb->db, "PRAGMA query_only=OFF", nullptr, nullptr, nullptr);
    }
    {
      OfflineServer s(cfg);
      auto resumed = call(s, "CMD_CREATE_USER")->getJSONObject("entry_info");
      check(resumed->getJSONArray("gameplayer_list")->length() == 2, "CREATE_USER resumes recruits after process restart");
      check(resumed->getJSONArray("squad_list")->length() > 0, "CREATE_USER resumes saved squads");
    }
    std::filesystem::create_directories(root + "/kill-save");
    pid_t child = fork();
    check(child >= 0, "spawn abrupt-exit save test");
    if (child == 0) {
      zs::ZiruDb db(root + "/kill-save/save.db");
      db.saveMeta("progress", "retained");
      zs::Prefs prefs(root + "/kill-save/prefs.json");
      prefs.edit().putString("recruited_players", "[1,2]").commit();
      _exit(0); // no C++ destructors, matching process termination after saving
    }
    int status = 0;
    check(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0, "abrupt-exit child saved");
    {
      zs::ZiruDb db(root + "/kill-save/save.db"); std::string progress;
      zs::Prefs prefs(root + "/kill-save/prefs.json");
      check(db.loadMeta("progress", progress) && progress == "retained" && prefs.getString("recruited_players", "") == "[1,2]", "DB and recruits survive exit without destructors");
    }
    std::filesystem::remove_all(root);
    std::cout << "PASS: gameplay, iOS booster layout/roundtrip, CREATE_USER resume, D1 stale CPU migration, failed writes and abrupt-exit persistence\n";
    return 0;
  } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << "\n"; return 1; }
}
