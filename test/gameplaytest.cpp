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

// ---- Card identity fixtures --------------------------------------------------
// A card's playing styles, AI playing styles, club, nation and own skills come
// from that exact card id only: its record, else its starter-squad entry, else
// its MyLeague rental entry. Nothing is invented and nothing is borrowed from
// another version of the same player.
static ObjP card(long long id, long long serial, int position, int style1, int style2, std::vector<int> ai,
                 std::vector<int> skills, int club, int nation, int size) {
  auto values = newArr(); for (int i = 0; i < size; ++i) values->put(0);
  values->put(0, position)->put(14, 70)->put(61, style1)->put(62, style2);
  for (int i = 0; i < 7; ++i) values->put(63 + i, ai[i]);
  for (int skill : skills) values->put(kZiruSkillParameters[skill], 1);
  return newObj()->put("gameplayer_id", newObj()->put("id", id)->put("serial", serial))
      ->put("play_style", newArr()->put(style1)->put(style2))
      ->put("initial_base_team_id", club)->put("initial_base_national_team_id", nation)
      ->put("parameters", values)->put("initial_booster_id_list", newArr()->put(0)->put(0)->put(0))
      ->put("additional_skill_id", newArr())
      ->put("level_info", Object::parse(R"({"level":1,"level_limit":1,"customize_point":0})"));
}
static ObjP bySerial(const ArrP& list, long long serial) {
  for (int i = 0; list && i < list->length(); ++i) {
    auto p = list->getJSONObject(i);
    if (p->optJSONObject("gameplayer")) p = p->getJSONObject("gameplayer");
    if (p->getJSONObject("gameplayer_id")->optLong("serial", -1) == serial) return p;
  }
  throw std::runtime_error("player " + std::to_string(serial) + " missing from response");
}
static ObjP byId(const ArrP& list, long long id) {
  for (int i = 0; list && i < list->length(); ++i) {
    auto p = list->getJSONObject(i);
    if (p->getJSONObject("gameplayer_id")->getLong("id") == id) return p;
  }
  throw std::runtime_error("card " + std::to_string(id) + " missing from response");
}
static int skillBit(const ObjP& p, int skill) { return p->getJSONArray("parameters")->getInt(kZiruSkillParameters[skill]); }
static bool sameStyles(const ObjP& p, int first, int second, const std::vector<int>& ai) {
  auto styles = p->optJSONArray("play_style"), values = p->getJSONArray("parameters");
  if (!styles || styles->length() != 2 || styles->getInt(0) != first || styles->getInt(1) != second) return false;
  if (values->getInt(61) != first || values->getInt(62) != second) return false;
  for (int i = 0; i < 7; ++i) if (values->getInt(63 + i) != ai[i]) return false;
  return true;
}

static void cardIdentityTests(const std::string& top) {
  const std::string root = top + "/card-identity";
  std::filesystem::create_directories(root + "/responses");
  const long long A = 14 + (1LL << 40);   // record: Fox in the Box + Basic, one AI style, Phenomenal Finishing
  const long long B = 14 + (2LL << 40);   // another version of player 14, starter entry only
  const long long S = 33 + (1LL << 40);   // starter entry only
  const long long K = 106788187843931LL;  // a card of player 44379 (De Bruyne), with a record
  const long long D = 55 + (1LL << 40);   // saved only: no record, starter or rental entry
  const long long R = 66 + (1LL << 40);   // MyLeague rental card only
  const long long C = 77, G = 78, Z = 79; // AI cards: The Destroyer + Basic, Basic + Defensive GK, Basic + Basic
  const long long E = 88 + (1LL << 40);   // record with two boosters
  const long long F = 99 + (1LL << 40);   // saved only, like D
  const std::vector<int> none = {0, 0, 0, 0, 0, 0, 0}, all = {1, 1, 1, 1, 1, 1, 1};
  const std::vector<int> aiA = {0, 0, 1, 0, 0, 0, 0}, aiS = {1, 0, 0, 0, 0, 0, 1}, aiK = {0, 1, 0, 0, 0, 0, 0};
  auto recordA = card(A, 0, 12, 3, 0, aiA, {29}, 0, 0, 149);
  auto recordK = card(K, 0, 9, 14, 0, aiK, {33}, 9, 0, 149);
  auto records = newObj()->put(std::to_string(A), newObj()->put("gameplayer", recordA))
      ->put(std::to_string(K), newObj()->put("gameplayer", recordK))
      ->put(std::to_string(C), newObj()->put("gameplayer", card(C, 0, 2, 9, 0, none, {57}, 108, 0, 149)))
      ->put(std::to_string(G), newObj()->put("gameplayer", card(G, 0, 0, 0, 17, none, {}, 108, 0, 149)))
      ->put(std::to_string(Z), newObj()->put("gameplayer", card(Z, 0, 2, 0, 0, none, {}, 108, 0, 149)));
  auto recordE = card(E, 0, 6, 20, 0, none, {3}, 108, 0, 149);
  recordE->put("initial_booster_id_list", newArr()->put(186)->put(136)->put(0));
  recordE->getJSONArray("parameters")->put(142, 186)->put(143, 136);
  records->put(std::to_string(E), newObj()->put("gameplayer", recordE));
  asset(root, "ZIRU_PLAYER_RECORDS.json", records);
  // Starter entries carry their owners' learned skills inside the parameters, like Konami's login data.
  auto starterS = card(S, 503, 7, 14, 0, aiS, {1, 6}, 50, 5, 145);
  starterS->put("additional_skill_id", newArr()->put(6));
  auto refs = newArr();
  for (long long serial : {501, 502, 503, 504, 505}) {
    long long id = serial == 501 ? A : serial == 502 ? B : serial == 503 ? S : serial == 504 ? K : D;
    refs->put(newObj()->put("gameplayer_id", newObj()->put("id", id)->put("serial", serial)));
  }
  auto squadData = newObj()->put("player_list", refs)->put("coach", newArr()->put(coachRef(100, 1000)))->put("squad_gameplan", newObj());
  auto starter = newObj()->put("main_squad_id", 9)
      ->put("gameplayer_list", newArr()->put(card(A, 501, 12, 3, 26, all, {29}, 157, 5, 145))
          ->put(card(B, 502, 12, 0, 0, none, {}, 0, 0, 145))->put(starterS)
          ->put(card(K, 504, 9, 14, 0, aiK, {33}, 173, 0, 145)))
      ->put("coach_list", newArr()->put(coachRef(100, 1000)))
      ->put("squad_list", newArr()->put(newObj()->put("squad_id", 9)->put("squad_data", squadData)
          ->put("base_team", newArr()->put(newObj()->put("base_team_id", 108)))));
  asset(root, "CmdLogin.json", newObj()->put("entry_info", starter));
  asset(root, "ZIRU_ML_RENTAL_LIST.json", newObj()->put("rental_player_list", newArr()->put(newObj()->put("point", 10)
      ->put("player_info", card(R, 0, 5, 20, 28, aiS, {44}, 61, 0, 145)))));
  asset(root, "CmdCreateUser.json", obj(R"({"user_code":"offline"})"));
  asset(root, "ZIRU_MATCH_TEAMS.json", obj(R"({"teams":{"108":{"base_team_id":108,"team_name":"Test","team_name_short":"TST","kits":[{"id":1,"shirt_rgb":[0,0,0]},{"id":2,"shirt_rgb":[255,255,255]}]}}})"));
  asset(root, "ZIRU_MATCH_PROFILES.json", newObj()->put("profiles", newArr()->put(obj(R"({"profile_id":1,"coordinates":[],"team_style":0,"base_team_id":108})")
      ->put("player_ids", newArr()->put(C)->put(G)->put(Z)))));
  asset(root, "ZIRU_GET_VSCOM_OPPONENT.json", obj(R"({"squad_data":{"squad_gameplan":{"formation":[],"match_plan_settings":{}}},"user_info":[{}]})"));
  asset(root, "ZIRU_START_GAME.json", obj(R"({"squad_data_home":{"coach":{"coach_id":{"id":999,"serial":999}}},"commentary_info":{}})"));
  asset(root, "CMD_USE_MYCLUB_ITEM.json", obj(R"({"result":"NOERR"})"));
  asset(root, "generic.json", obj(R"({"result":"NOERR"})"));
  Config cfg; cfg.assetsDir = root; cfg.dataDir = root + "/save";
  std::filesystem::create_directories(cfg.dataDir);
  OfflineServer s(cfg);
  // Rows as an older build saved them: an invented defensive style, every AI
  // style, a guessed club and a skill borrowed from another version of the player.
  s.ziruDb->savePlayer(501, A, card(A, 501, 12, 3, 26, all, {29, 0}, 157, 5, 145)->toString());
  s.ziruDb->savePlayer(502, B, card(B, 502, 12, 3, 26, all, {29}, 157, 0, 145)->toString());
  s.ziruDb->savePlayer(504, K, card(K, 504, 9, 14, 30, all, {33}, 173, 0, 145)->toString());
  auto savedD = card(D, 505, 4, 0, 9, aiK, {2}, 33, 0, 145);
  savedD->getJSONArray("parameters")->put(61, 5)->put(62, 5);
  s.ziruDb->savePlayer(505, D, savedD->toString());

  auto roster = call(s, "CMD_LOGIN")->getJSONObject("entry_info")->getJSONArray("gameplayer_list");
  auto loginA = bySerial(roster, 501), loginB = bySerial(roster, 502), loginS = bySerial(roster, 503);
  auto loginK = bySerial(roster, 504), loginD = bySerial(roster, 505);
  check(sameStyles(loginA, 3, 0, aiA), "login: the record's styles and AI styles replace invented ones");
  check(skillBit(loginA, 29) == 1 && skillBit(loginA, 0) == 0, "login: the card's own skills only, no borrowed skill");
  check(loginA->getInt("initial_base_team_id") == 0 && loginA->getInt("initial_base_national_team_id") == 0,
      "login: club and nation come from the record, even when it has none");
  check(sameStyles(loginB, 0, 0, none) && skillBit(loginB, 29) == 0 && loginB->getInt("initial_base_team_id") == 0,
      "login: a card without a record uses its own starter entry, never another version of the player");
  check(sameStyles(loginS, 14, 0, aiS) && loginS->getInt("initial_base_team_id") == 50 && loginS->getInt("initial_base_national_team_id") == 5,
      "login: a starter-only card keeps its starter styles, AI styles, club and nation");
  check(skillBit(loginS, 1) == 1 && skillBit(loginS, 6) == 1 && loginS->getJSONArray("additional_skill_id")->getInt(0) == 6,
      "login: a starter card keeps its own skill and the skill its owner learned");
  auto params = loginK->getJSONArray("parameters");
  check(sameStyles(loginK, 14, 0, aiK) && loginK->getInt("initial_base_team_id") == 9, "login: card of player 44379 uses its record");
  check(params->getInt(5) == 0 && params->getInt(9) == 0 && params->getInt(14) == 70,
      "login: no position or rating change for player 44379");
  check(sameStyles(loginD, 0, 9, aiK) && skillBit(loginD, 2) == 1 && loginD->getInt("initial_base_team_id") == 33,
      "login: a card nobody knows keeps its saved Basic + defensive style; parameters 61/62 follow play_style");
  check(sameStyles(Object::parse(*s.loadPlayer(501)), 3, 0, aiA) && Object::parse(*s.loadPlayer(501))->getInt("initial_base_team_id") == 0,
      "save: the corrected card is written back");
  check(sameStyles(Object::parse(*s.loadPlayer(502)), 0, 0, none), "save: the borrowed styles are removed from the save");
  auto detail = call(s, "CMD_GET_MYCLUB_PLAYER_DATA", newObj()->put("gameplayer_id", newObj()->put("id", A)->put("serial", 501)));
  detail->put("play_style", newArr()->put(3)->put(0));
  check(sameStyles(detail, 3, 0, aiA) && skillBit(detail, 0) == 0, "player detail: the record's styles, AI styles and skills");
  auto entry = call(s, "CMD_GET_MYCLUB_ENTRY_INFO")->getJSONObject("entry_info")->getJSONArray("gameplayer_list");
  check(sameStyles(bySerial(entry, 501), 3, 0, aiA) && sameStyles(bySerial(entry, 503), 14, 0, aiS), "entry info: same cards as login");

  auto opponent = call(s, "CMD_GET_VSCOM_OPPONENT");
  auto ai = opponent->getJSONArray("player_list");
  check(sameStyles(byId(ai, C), 9, 0, none), "opponent: The Destroyer + Basic stays as printed, no invented AI styles");
  check(sameStyles(byId(ai, G), 0, 17, none), "opponent: Basic + Defensive GK is a real pair and is sent as is");
  check(sameStyles(byId(ai, Z), 0, 0, none), "opponent: Basic + Basic is sent as is");
  auto start = call(s, "CMD_START_GAME", newObj()->put("game_id", (long long)opponent->getLong("game_id")));
  auto home = start->getJSONObject("squad_data_home")->getJSONArray("player_list");
  auto away = start->getJSONObject("squad_data_away")->getJSONArray("player_list");
  check(sameStyles(bySerial(home, 501), 3, 0, aiA) && sameStyles(bySerial(home, 502), 0, 0, none) &&
        sameStyles(bySerial(home, 503), 14, 0, aiS) && sameStyles(bySerial(home, 505), 0, 9, aiK),
      "match start: the user's players carry the same styles as their cards");
  check(sameStyles(byId(away, C), 9, 0, none) && sameStyles(byId(away, G), 0, 17, none), "match start: AI cards as printed");


  // Saves written by older builds: a learned Snap Strike (25) / Attack Trigger (68)
  // stored as 72/73 with booster slots overwritten by 1, a learned Dummy skill,
  // recruited-player copies and a cached match squad.
  auto oldSave = [&](Config c) {
    std::filesystem::create_directories(c.dataDir);
    auto srv = std::make_unique<OfflineServer>(c);
    auto e = card(E, 601, 6, 20, 27, all, {3}, 0, 0, 145);
    e->put("additional_skill_id", newArr()->put(72)->put(73)->put(9));
    e->put("initial_booster_id_list", newArr()->put(186)->put(136)->put(0));
    e->getJSONArray("parameters")->put(142, 1)->put(143, 1)->put(kZiruSkillParameters[9], 1);
    srv->ziruDb->savePlayer(601, E, e->toString());
    auto f = card(F, 602, 6, 20, 0, none, {3, 9}, 0, 0, 145);
    f->put("additional_skill_id", newArr()->put(9))->put("original_parameters", Array::parse(card(F, 602, 6, 20, 0, none, {3}, 0, 0, 145)->getJSONArray("parameters")->toString()));
    srv->ziruDb->savePlayer(602, F, f->toString());
    auto recruited = card(E, 603, 6, 20, 27, all, {3}, 0, 0, 145);
    recruited->put("additional_skill_id", newArr()->put(73));
    srv->sp("almanac_state").edit().putString("ziru_skills_601", "72,73,9")
        .putString("recruited_players", newArr()->put(newObj()->put("gameplayer", recruited))->toString()).commit();
    srv->ziruDb->saveMeta("z27_active_match", R"({"game_id":5,"home_squad":{"player_list":[{"gameplayer_id":{"id":1,"serial":601},"parameters":[0]}]}})");
    return srv;
  };
  {
    Config repairCfg = cfg; repairCfg.dataDir = root + "/repair";
    auto srv = oldSave(repairCfg);
    auto list = call(*srv, "CMD_LOGIN")->getJSONObject("entry_info")->getJSONArray("gameplayer_list");
    auto e = bySerial(list, 601), f = bySerial(list, 602);
    auto learned = e->getJSONArray("additional_skill_id");
    check(learned->length() == 2 && learned->getInt(0) == 25 && learned->getInt(1) == 68,
        "old save: learned 72/73 are Snap Strike and Attack Trigger again, the Dummy skill is gone");
    check(skillBit(e, 25) == 1 && skillBit(e, 68) == 1 && skillBit(e, 9) == 0 && skillBit(e, 3) == 1,
        "old save: learned skills keep working, the Dummy skill flag is cleared");
    auto values = e->getJSONArray("parameters");
    check(values->getInt(142) == 186 && values->getInt(143) == 136 && values->getInt(144) == 0,
        "old save: booster slots written as 1 get the owned boosters back");
    check(sameStyles(e, 20, 0, none), "old save: invented styles are replaced by the record's");
    check(srv->sp("almanac_state").getString("ziru_skills_601", "") == "25,68", "old save: the learned-skill list in prefs is repaired");
    check(f->getJSONArray("additional_skill_id")->length() == 0 && skillBit(f, 9) == 0 && skillBit(f, 3) == 1,
        "old save: a card without a record loses the learned Dummy skill, not its own skill");
    auto recruited = Array::parse(srv->sp("almanac_state").getString("recruited_players", "[]"))->getJSONObject(0)->getJSONObject("gameplayer");
    check(sameStyles(recruited, 20, 0, none) && recruited->getJSONArray("additional_skill_id")->getInt(0) == 68,
        "old save: recruited player copies are corrected too");
    check(!srv->z27Meta("z27_active_match")->has("home_squad"), "old save: the cached match squad is dropped and rebuilt at match start");
    auto saved = Object::parse(*srv->loadPlayer(601));
    check(saved->getJSONArray("parameters")->getInt(142) == 186 && saved->getJSONArray("additional_skill_id")->getInt(0) == 25,
        "old save: the repaired player is saved");
  }
  {
    Config wireCfg = cfg; wireCfg.dataDir = root + "/repair-ios"; wireCfg.iosPlayerLayout = true;
    auto srv = oldSave(wireCfg);
    auto e = bySerial(call(*srv, "CMD_LOGIN")->getJSONObject("entry_info")->getJSONArray("gameplayer_list"), 601);
    auto wire = e->getJSONArray("parameters");
    check(wire->length() == 147 && wire->getInt(142) == 0 && wire->getInt(143) == 0 && wire->getInt(144) == 186 && wire->getInt(145) == 136,
        "iOS wire: Dummy skill flags stay off and the boosters are in their iOS slots");
    auto learned = e->getJSONArray("additional_skill_id");
    check(learned->getInt(0) == iosSkillId(25, true) && learned->getInt(1) == iosSkillId(68, true), "iOS wire: learned skills use the client's numbers");
    check(wire->getInt(kZiruSkillParameters[25]) == 1 && wire->getInt(kZiruSkillParameters[68]) == 1, "iOS wire: the learned skills' own flags are on");
  }

  // Skill Training: only normal skills; a replaced skill stops working.
  auto train = [&](const char* type, long long id, long long serial, ObjP extra) {
    auto req = newObj()->put("gameplayer_id_list", newArr()->put(newObj()->put("id", id)->put("serial", serial)))->put("training_index", 0)
        ->put("item_list", newArr()->put(newObj()->put("present_num", 1)->put("item", newObj()->put("item_type", type))));
    for (const auto& k : extra->keys()) req->put(k, extra->get(k));
    return call(s, "CMD_USE_MYCLUB_ITEM", req)->getJSONObject("skill_training_result")->getInt("additional_skill_id");
  };
  auto skillCount = [](const ObjP& p) { int n = 0; for (int i = 70; i < 142; ++i) n += p->getJSONArray("parameters")->getInt(i); return n; };
  const std::vector<std::string> categories = {"SHOOT", "PASS", "DRIBBLE", "DEFENCE", "OTHER"};
  for (int k = 0; k < 120; ++k) {
    int awarded = train(k % 2 ? "SKILL_TRAINING" : "SKILL_TRAINING_CATEGORY", A, 501,
        newObj()->put("skill_training_category_type", categories[k % 5]));
    check(isZiruTrainableSkill(awarded), "Skill Training never awards a special or Dummy skill");
  }
  for (int refused : {29, 9, 72, 73}) {
    int awarded = train("SKILL_TRAINING_SELECT", A, 501, newObj()->put("skill_id", refused));
    check(awarded != refused && isZiruTrainableSkill(awarded), "a special or Dummy skill cannot be selected");
  }
  check(train("SKILL_TRAINING_SELECT", A, 501, newObj()->put("skill_id", 44)) == 44, "a normal skill can be selected");
  auto trainedA = Object::parse(*s.loadPlayer(501));
  check(skillCount(trainedA) == 2 && skillBit(trainedA, 29) == 1 && skillBit(trainedA, 44) == 1,
      "retraining one slot leaves the card's own skill plus the one learned skill");
  check(sameStyles(trainedA, 3, 0, aiA), "training keeps the card's styles");
  // A card nobody knows (D, own skill 2): the replaced skill's flag is cleared too.
  check(train("SKILL_TRAINING_SELECT", D, 505, newObj()->put("skill_id", 0)) == 0, "unknown card: first skill learned");
  check(train("SKILL_TRAINING_SELECT", D, 505, newObj()->put("skill_id", 1)) == 1, "unknown card: slot retrained");
  for (int k = 0; k < 30; ++k) train("SKILL_TRAINING", D, 505, newObj());
  auto trainedD = Object::parse(*s.loadPlayer(505));
  check(skillCount(trainedD) == 2 && skillBit(trainedD, 2) == 1 && trainedD->getJSONArray("additional_skill_id")->length() == 1,
      "unknown card: retraining leaves its own skill plus one learned skill");
  auto choices = call(s, "CMD_GET_MYCLUB_SELECT_SKILL")->getJSONArray("select_skill_list");
  check(choices->length() == 44, "the selection list has the 44 normal skills");
  for (int i = 0; i < choices->length(); ++i) check(isZiruTrainableSkill(choices->getInt(i)), "the selection list offers normal skills only");
  {
    Config wireCfg = cfg; wireCfg.dataDir = root + "/select-ios"; wireCfg.iosPlayerLayout = true;
    std::filesystem::create_directories(wireCfg.dataDir);
    OfflineServer w(wireCfg);
    auto wire = call(w, "CMD_GET_MYCLUB_SELECT_SKILL")->getJSONArray("select_skill_list");
    for (int i = 0; i < wire->length(); ++i) check(isZiruTrainableSkill(iosSkillId(wire->getInt(i), false)), "iOS selection list offers normal skills only");
  }
  // Progression reset keeps the owned boosters of a 145-value save.
  auto boosted = Object::parse(*s.loadPlayer(501));
  boosted->getJSONArray("parameters")->put(142, 12);
  boosted->put("initial_booster_id_list", newArr()->put(12)->put(0)->put(0));
  s.ziruDb->savePlayer(501, A, boosted->toString());
  auto reset = call(s, "CMD_USE_MYCLUB_CAREERPLAN", newObj()->put("gameplayer_use_list", newArr()->put(
      newObj()->put("gameplayer_id", newObj()->put("id", A)->put("serial", 501))->put("use_list", newArr()))));
  auto resetA = reset->getJSONArray("careerplan_result_list")->getJSONObject(0)->getJSONObject("gameplayer");
  check(resetA->getJSONArray("parameters")->getInt(142) == 12, "progression reset keeps the boosters");
  check(sameStyles(resetA, 3, 0, aiA) && skillBit(resetA, 44) == 1, "progression reset keeps the styles and the learned skill");
  check(!s.z27Player(newObj()->put("id", 424242LL)->put("serial", 0LL)), "an unknown card is not built from another player");
  auto rental = s.z27Player(newObj()->put("id", R)->put("serial", 0LL));
  check(rental && sameStyles(rental, 20, 28, aiS) && skillBit(rental, 44) == 1 && rental->getInt("initial_base_team_id") == 61,
      "a MyLeague rental card uses its own rental entry");
}

int main() {
  try {
    zs::setVerbose(false);
    {
      Value coaches(newArr()->put(newObj()->put("coach_id", newObj()->put("id", 17609097478250LL))
          ->put("ability_id", newArr()->put(11)->put(14)->put(7)->put(24)))
          ->put(newObj()->put("coach_id", newObj()->put("id", 17608560707469LL))
          ->put("ability_id", newArr()->put(7)->put(13)->put(17)->put(28)))
          ->put(newObj()->put("coach_id", newObj()->put("id", 123))
          ->put("ability_id", newArr()->put(7)->put(13)->put(17)->put(28))));
      adaptIosPlayerData(coaches, true);
      check(coaches.a->getJSONObject(0)->getJSONArray("ability_id")->getInt(3) == 12, "Conte second link matches native Coach.bin");
      check(coaches.a->getJSONObject(1)->getJSONArray("ability_id")->getInt(3) == 14, "Mourinho second link matches native Coach.bin");
      check(coaches.a->getJSONObject(2)->getJSONArray("ability_id")->getInt(3) == 28, "other coaches retain their link ID");
      adaptIosPlayerData(coaches, true);
      check(coaches.a->getJSONObject(0)->getJSONArray("ability_id")->getInt(3) == 12, "coach repair is idempotent");
    }
    // Independent table extracted from iOS Player::GetDataLabel(SkillCard),
    // VA 0x10578bc00. Compare semantic parameter labels for every source skill.
    const int iosSkillLabels[74] = {
      70,107,71,72,73,108,74,75,135,136,102,116,124,130,140,76,121,77,
      120,101,78,103,104,109,127,143,134,79,80,81,114,128,82,110,83,84,
      119,85,86,98,117,118,122,141,87,88,105,89,90,99,100,138,139,126,
      129,91,92,93,106,111,112,113,125,137,115,94,123,132,142,95,131,96,97,133
    };
    auto skillIds = newArr(); for (int i = 0; i < 72; ++i) skillIds->put(i);
    Value skillWire(newObj()->put("additional_skill_id", skillIds)->put("skillcard_id_list", skillIds)
        ->put("skill_training_result", newObj()->put("additional_skill_id", 67)));
    adaptIosPlayerData(skillWire, true);
    for (int i = 0; i < 72; ++i) {
      int target = skillWire.o->getJSONArray("additional_skill_id")->getInt(i);
      check(iosSkillLabels[target] == kZiruSkillParameters[i], "skill enum must refer to the same actual skill parameter");
      check(skillWire.o->getJSONArray("skillcard_id_list")->getInt(i) == target, "training list uses the same skill mapping");
    }
    check(skillWire.o->getJSONObject("skill_training_result")->getInt("additional_skill_id") == 69, "training result maps skill scalar");
    adaptIosPlayerData(skillWire, false);
    for (int i = 0; i < 72; ++i) check(skillWire.o->getJSONArray("additional_skill_id")->getInt(i) == i, "all skill IDs roundtrip to stable save format");
    {
      auto params = newArr(); for (int i = 0; i < 149; ++i) params->put(0);
      params->put(71, 1); params->put(142, 1); params->put(143, 1);
      Value owned(newObj()->put("gameplayer_id", newObj()->put("id", 4073))
          ->put("parameters", params)->put("additional_skill_id", newArr()));
      adaptIosPlayerData(owned, true);
      auto skills = owned.o->getJSONArray("skillcard_id_list");
      check(skills->length() == 1 && skills->getInt(0) == 2,
          "owned-player menu receives Double Touch in the native skill enum");
      auto wire = owned.o->getJSONArray("parameters");
      check(wire->getInt(142) == 0 && wire->getInt(143) == 0,
          "booster ID one must not invent two extra skill flags");
      check(wire->getInt(144) == 1 && wire->getInt(145) == 1,
          "booster IDs survive skill-list reconstruction");
      check(!isZiruSkillParameter(kZiruSkillParameters[72]) && !isZiruSkillParameter(kZiruSkillParameters[73]),
          "new skills are represented by explicit IDs, never by overwriting save boosters");
    }
    char temp[] = "/tmp/pes-gameplay-XXXXXX";
    check(mkdtemp(temp), "temporary directory");
    std::string root(temp);
    std::filesystem::create_directories(root + "/responses");
    Config cfg; cfg.assetsDir = root; cfg.dataDir = root + "/save";
    {
      Config stadiumCfg = cfg; stadiumCfg.downloadDir = root + "/stadiums";
      std::filesystem::create_directories(stadiumCfg.downloadDir);
      for (const char* ext : {"pak", "ucas", "utoc"})
        std::ofstream(stadiumCfg.downloadDir + "/pc3099_mobile_ios." + ext) << "custom";
      std::ofstream(stadiumCfg.downloadDir + "/pc3002_mobile_ios.pak") << "incomplete";
      OfflineServer server(stadiumCfg);
      auto list = call(server, "CMD_GET_STADIUM_DATA")->getJSONArray("data_list");
      check(list->length() == 3, "advertise complete installed stadiums only, including new IDs");
      check(list->getJSONObject(0)->getString("name") == "pc3099_mobile_ios.pak", "custom stadium ID discovered");
      check(list->getJSONObject(1)->getInt("size") == 6, "stadium metadata uses installed size");
      auto hash = list->getJSONObject(1)->getString("hash");
      std::ofstream(stadiumCfg.downloadDir + "/pc3099_mobile_ios.ucas") << "edited";
      list = call(server, "CMD_GET_STADIUM_DATA")->getJSONArray("data_list");
      check(list->getJSONObject(1)->getString("hash") != hash, "same-size replacement invalidates stadium hash");
    }
    {
      std::filesystem::create_directories(root + "/livedata");
      std::ofstream(root + "/livedata/pc9999_mobile_ios.ucas") << "native";
      std::ofstream(root + "/livedata/pc9999_mobile_and.ucas") << "old";
      std::ofstream(root + "/livedata/dt870_mobile_and.cpk") << "fixed";
      Config ios = cfg; ios.iosPlayerLayout = true; ios.dropAndroidLivedata = false; ios.livedataDir = root + "/livedata";
      OfflineServer server(ios);
      auto data = obj(R"({"livedata_info":{"livedata_list":[{"name":"pc9999_mobile_and.ucas","size":3},{"name":"dt870_mobile_and.cpk","size":99}],"version_file":"{\"dt880\":[{\"name\":\"pc9999\"}]}"}})");
      server.patchLocalLivedataUrls(data);
      auto info = data->getJSONObject("livedata_info");
      auto list = info->getJSONArray("livedata_list");
      auto version = Object::parse(info->getString("version_file"));
      check(list->getJSONObject(0)->getInt("size") == 6 && list->getJSONObject(0)->getString("hash").size() == 64,
          "livedata list uses actual native bytes, not Android metadata");
      check(version->getInt("dt870FileSize") == 5 && version->getString("dt870Hash") == list->getJSONObject(1)->getString("hash"),
          "repaired database hash is also updated in the version file");
      check(version->getJSONArray("dt880")->getJSONObject(0)->getInt("ucasFileSize") == 6,
          "native texture container size is also updated in the version file");
      auto agueroParams = newArr();
      for (int i = 0; i < 149; ++i) agueroParams->put(0);
      auto aguero = newObj()->put("gameplayer_id", newObj()->put("id", 88044682118054LL))
          ->put("parameters", agueroParams)->put("play_style", newArr()->put(1)->put(0));
      server.z27ApplyAppearanceAndClub(aguero);
      check(agueroParams->getInt(49) == 7 && agueroParams->getInt(50) == 1 && agueroParams->getInt(56) == 14,
          "Aguero's missing appearance comes from his known base preset");
      agueroParams->put(50, 9);
      server.z27ApplyAppearanceAndClub(aguero);
      check(agueroParams->getInt(50) == 9, "existing appearance customization is preserved");
    }
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
    {
      // The card itself has Double Touch (skill 2, parameter 71); the owner learned 25 and 68.
      auto intrinsic = Object::parse(p->toString());
      intrinsic->getJSONArray("parameters")->put(71, 1);
      asset(root, "ZIRU_PLAYER_RECORDS.json", newObj()->put("1", newObj()->put("gameplayer", intrinsic)));
      Config skillCfg = cfg; skillCfg.dataDir = root + "/skill-display";
      std::filesystem::create_directories(skillCfg.dataDir);
      OfflineServer server(skillCfg);
      auto owned = Object::parse(p->toString());
      auto params = Array::parse(parameters->toString());
      params->put(14, 91); params->put(71, 1); params->put(142, 42);
      owned->put("parameters", params)->put("additional_skill_id", newArr()->put(25)->put(68));
      server.ziruDb->savePlayer(101, 1, owned->toString());
      auto detail = call(server, "CMD_GET_MYCLUB_PLAYER_DATA", newObj()->put("gameplayer_id", newObj()->put("id", 1)->put("serial", 101)));
      check(detail->getJSONArray("parameters")->getInt(14) == 91,
          "player-detail endpoint uses owned progression rather than catalog stats");
      auto cards = detail->getJSONArray("skillcard_id_list");
      bool intrinsicShown = false;
      for (int i = 0; i < cards->length(); ++i) intrinsicShown |= cards->getInt(i) == 2;
      check(intrinsicShown, "owned intrinsic skills appear outside matches");
      auto extra = detail->getJSONArray("additional_skill_id");
      check(extra->getInt(0) == 25 && extra->getInt(1) == 68,
          "stored internal skill IDs 25/68 are not remapped a second time");
      auto player = server.z27Player(newObj()->put("id", 1)->put("serial", 101));
      check(player->getJSONArray("additional_skill_id")->getInt(0) == 25,
          "match player retains the same internal learned skill identity");
      check(player->getJSONArray("parameters")->getInt(142) == 42,
          "skill synchronization preserves booster data");
      asset(root, "ZIRU_PLAYER_RECORDS.json", newObj()->put("1", newObj()->put("gameplayer", p)));
    }
    asset(root, "ZIRU_GET_VSCOM_OPPONENT.json", obj(R"({"squad_data":{"squad_gameplan":{"formation":[],"match_plan_settings":{}}},"user_info":[{}]})"));
    auto start = obj(R"({"squad_data_home":{"coach":{"coach_id":{"id":999,"serial":999}}},"commentary_info":{}})");
    asset(root, "ZIRU_START_GAME.json", start); asset(root, "ZIRU_ML_START_GAME.json", start);
    asset(root, "ZIRU_GET_GAMERESULT_LIST.json", obj(R"({"game_result_list":[{"player":[{},{}]}]})"));
    asset(root, "ZIRU_GET_VSCOM_GAME_RESULT_TEMPLATE.json", obj(R"({"game_end_result":{"match_result":{"player_data":{}},"extra_match_result":{"league_match_result":{"league_info":{"com_level_info":{},"round_match_result":{}}}}}})"));
    asset(root, "cmd_get_league_com_info.json", obj(R"({"league_info":{"com_level_info":{},"round_match_result":{}}})"));
    asset(root, "generic.json", obj(R"({"result":"NOERR"})"));
    {
      OfflineServer s(cfg);
      auto a = call(s, "CMD_CREATE_MYCLUB_SQUAD", newObj()->put("name", "First tactic"))->getJSONObject("new_squad_data");
      auto b = call(s, "CMD_CREATE_MYCLUB_SQUAD", newObj()->put("name", "Second tactic"))->getJSONObject("new_squad_data");
      check(a->getInt("squad_id") != b->getInt("squad_id"), "creating two tactics must allocate distinct IDs");
      check(a->getString("name") == "First tactic" && a->optJSONObject("squad_data") && a->getJSONArray("base_team")->length() == 1,
          "create result matches native MyclubSquad wrapper");
      check(Object::parse(*s.loadSquad(a->getInt("squad_id")))->getString("squad_name") == "First tactic",
          "second create must preserve first tactic");
    }
    {
      OfflineServer s(cfg);
      check(s.z27State()->getInt("division") == 10, "starts at D10 by default");
      s.ziruMyLeagueSetActive(1);
      call(s, "CMD_GET_LEAGUE_COM_INFO");
      check(!s.ziruMyLeagueIsActive(), "entering AI rank exits a previous MyLeague session");
      s.ziruDb->saveMeta("z27_division_state", R"({"division":10,"best":10,"point":0,"matches":0,"win":0,"draw":0,"lose":0,"sequence":0,"completed":{}})");
      s.ziruDb->saveMeta("z27_div10_reset_v3", "1");
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
      s.ziruDb->saveMeta("z27_div10_reset_v3", "");
      auto active = s.z27Meta("z27_active_match");
      active->put("division", 10)->put("cpu_level", "NORMAL");
      s.ziruDb->saveMeta("z27_active_match", active->toString());
      check(sqlite3_exec(s.ziruDb->db, "DELETE FROM meta WHERE k='z27_div10_reset_v3'", nullptr, nullptr, nullptr) == SQLITE_OK, "prepare legacy save");
      check(s.z27State()->getInt("division") == 10, "legacy rank migrates to D10");
      auto selected = call(s, "CMD_GET_VSCOM_OPPONENT");
      check(selected->getString("cpu_level") == "NORMAL" && s.z27Active()->getInt("division") == 10, "cached opponent migrates with D10");
      active = s.z27Meta("z27_active_match");
      active->put("division", 10)->put("cpu_level", "NORMAL");
      s.ziruDb->saveMeta("z27_active_match", active->toString());
      check(call(s, "CMD_GET_VSCOM_OPPONENT")->getString("cpu_level") == "NORMAL", "already-migrated D10 saves maintain D10 snapshots");
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
      // The card has every skill but skill 0 by itself, so 0 is the only skill left to train.
      auto builtIn = Object::parse(p->toString());
      auto builtInParams = builtIn->getJSONArray("parameters");
      while (builtInParams->length() < 149) builtInParams->put(0);
      for (int id = 0; id < 72; ++id) builtInParams->put(kZiruSkillParameters[id], id == 0 ? 0 : 1);
      builtIn->put("initial_booster_id_list", newArr()->put(12)->put(0)->put(0));
      asset(root, "ZIRU_PLAYER_RECORDS.json", newObj()->put("1", newObj()->put("gameplayer", builtIn)));
      s.ziruPlayerRecordsCache.reset();
      auto owned = Object::parse(*s.loadPlayer(101));
      owned->getJSONObject("gameplayer_id")->put("serial", 303);
      auto params = owned->getJSONArray("parameters");
      for (int id = 0; id < 72; ++id) params->put(kZiruSkillParameters[id], id == 0 ? 0 : 1);
      owned->put("additional_skill_id", newArr());
      s.ziruDb->savePlayer(303, 1, owned->toString());
      asset(root, "CMD_USE_MYCLUB_ITEM.json", obj(R"({"result":"NOERR"})"));
      auto req = newObj()->put("gameplayer_id_list", newArr()->put(owned->getJSONObject("gameplayer_id")))
          ->put("training_index", 0)->put("item_list", newArr()->put(newObj()->put("present_num", 1)
          ->put("item", newObj()->put("item_type", "SKILL_TRAINING"))));
      auto itemResp = call(s, "CMD_USE_MYCLUB_ITEM", req);
      auto awarded = itemResp->getJSONObject("skill_training_result")->getInt("additional_skill_id");
      check(awarded == 0, "training excludes all built-in skills; ID zero is a real skill");
      check(itemResp->getJSONArray("item_list")->length() > 0, "item_list replenished in CMD_USE_MYCLUB_ITEM");
    }
    {
      OfflineServer s(cfg);
      auto players = call(s, "CMD_LOGIN")->getJSONObject("entry_info")->getJSONArray("gameplayer_list");
      bool retainedZero = false;
      for (int i = 0; i < players->length(); ++i) {
        auto p = players->getJSONObject(i);
        if (p->getJSONObject("gameplayer_id")->getLong("serial") == 303) {
          auto skills = p->getJSONArray("additional_skill_id");
          retainedZero = skills->length() == 1 && skills->getInt(0) == 0;
          check(p->getJSONArray("parameters")->getInt(kZiruSkillParameters[0]) == 1, "skill zero parameter bit set in roster");
        }
      }
      check(retainedZero, "valid skill zero survives save, process restart and login");

      // Verify skill zero parameter bit and additional skill survive z27Restore via CMD_GET_MYCLUB_ENTRY_INFO
      auto entryResp = call(s, "CMD_GET_MYCLUB_ENTRY_INFO");
      auto entryPlayers = entryResp->getJSONObject("entry_info")->getJSONArray("gameplayer_list");
      bool entryHasSkill = false;
      for (int i = 0; i < entryPlayers->length(); ++i) {
        auto p = entryPlayers->getJSONObject(i);
        if (p->getJSONObject("gameplayer_id")->getLong("serial") == 303) {
          auto skills = p->optJSONArray("additional_skill_id");
          if (skills && skills->length() == 1 && skills->getInt(0) == 0) entryHasSkill = true;
          check(p->getJSONArray("parameters")->getInt(kZiruSkillParameters[0]) == 1, "skill zero parameter bit survives post-match entry info");
        }
      }
      check(entryHasSkill, "additional skill survives post-match entry info");
    }
    {
      OfflineServer s(cfg);
      auto req = newObj()->put("achievement_item_info_list", newArr()->put(newObj()->put("achievement_id", 101)->put("sheet_id", 1)->put("level", 1)));
      auto resp = call(s, "CMD_RECV_MYCLUB_ACHIEVEMENT", req);
      check(resp->getJSONArray("achievement_list")->length() > 0, "achievement_list returned");
      check(resp->getJSONArray("receive_result_list")->length() > 0, "receive_result_list returned");
      check(resp->getJSONArray("receive_result_list")->getJSONObject(0)->getString("is_success") == "YES", "reward claimed successfully");
    }
    cardIdentityTests(root);
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
    std::cout << "PASS: gameplay, booster layout, all 72 skill mappings/roundtrip/training/skill-zero restart, CREATE_USER resume, D1 migration, failed writes and abrupt-exit persistence\n";
    return 0;
  } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << "\n"; return 1; }
}
