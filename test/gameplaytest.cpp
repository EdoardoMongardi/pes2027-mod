// End-to-end encrypted commands, synthetic fixtures (no game assets required).
#include "../core/server.hpp"
#include <filesystem>
#include <functional>
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

// The match uses CMD_START_GAME's parameters for the user's own players, so they
// must be exactly the card's parameters (styles, AI styles, skills, everything).
static std::string paramDiff(const ObjP& card, const ObjP& match) {
  auto a = card->getJSONArray("parameters"), b = match->getJSONArray("parameters");
  if (a->length() != b->length()) return "length " + std::to_string(a->length()) + " vs " + std::to_string(b->length());
  for (int i = 0; i < a->length(); ++i)
    if (a->optInt(i, -1) != b->optInt(i, -1)) return "index " + std::to_string(i);
  return "";
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
  const long long T = 7511 + (5LL << 40); // starter entry sharing serial 501 with A; player 7511 has a weak-foot fix
  const long long A2 = 14 + (6LL << 40);  // another version of player 14 (A's player), record only
  const long long E2 = 88 + (7LL << 40);  // starter entry only; the first record of its player (E) has boosters
  const long long X = 120 + (1LL << 40);  // record: has 2 of the 7 normal defensive skills
  const long long FULL1 = 121 + (1LL << 40), FULL2 = 122 + (1LL << 40);  // records holding (nearly) every normal skill
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
  records->put(std::to_string(A2), newObj()->put("gameplayer", card(A2, 0, 12, 11, 0, aiK, {5}, 0, 0, 149)));
  records->put(std::to_string(X), newObj()->put("gameplayer", card(X, 0, 9, 9, 0, none, {55, 56}, 108, 0, 149)));
  std::vector<int> allNormal, allButFive;
  for (int skill = 0; skill < 72; ++skill) if (isZiruTrainableSkill(skill)) { allNormal.push_back(skill); if (skill > 4) allButFive.push_back(skill); }
  records->put(std::to_string(FULL1), newObj()->put("gameplayer", card(FULL1, 0, 9, 9, 0, none, allButFive, 108, 0, 149)));
  records->put(std::to_string(FULL2), newObj()->put("gameplayer", card(FULL2, 0, 9, 9, 0, none, allNormal, 108, 0, 149)));
  asset(root, "ZIRU_PLAYER_RECORDS.json", records);
  // Starter entries carry their owners' learned skills inside the parameters, like Konami's login data.
  auto starterS = card(S, 503, 7, 14, 0, aiS, {1, 6}, 50, 5, 145);
  starterS->put("additional_skill_id", newArr()->put(6));
  auto refs = newArr();
  for (long long serial : {501, 502, 503, 504, 505}) {
    long long id = serial == 501 ? A : serial == 502 ? B : serial == 503 ? S : serial == 504 ? K : D;
    refs->put(newObj()->put("gameplayer_id", newObj()->put("id", id)->put("serial", serial)));
  }
  refs->put(newObj()->put("gameplayer_id", newObj()->put("id", T)->put("serial", 501)));
  auto starterE2 = card(E2, 509, 6, 20, 0, none, {3}, 108, 0, 145);
  starterE2->put("initial_booster_id_list", newArr()->put(12)->put(0)->put(0));
  starterE2->getJSONArray("parameters")->put(142, 12);
  auto squadData = newObj()->put("player_list", refs)->put("coach", newArr()->put(coachRef(100, 1000)))->put("squad_gameplan", newObj());
  auto starter = newObj()->put("main_squad_id", 9)
      ->put("gameplayer_list", newArr()->put(card(A, 501, 12, 3, 26, all, {29}, 157, 5, 145))
          ->put(card(B, 502, 12, 0, 0, none, {}, 0, 0, 145))->put(starterS)
          ->put(card(K, 504, 9, 14, 0, aiK, {33}, 173, 0, 145))->put(card(T, 501, 10, 4, 0, none, {7}, 0, 0, 145))->put(starterE2))
      ->put("coach_list", newArr()->put(coachRef(100, 1000)))
      ->put("squad_list", newArr()->put(newObj()->put("squad_id", 9)->put("squad_data", squadData)
          ->put("base_team", newArr()->put(newObj()->put("base_team_id", 108)))));
  asset(root, "CmdLogin.json", newObj()->put("entry_info", starter));
  // A rental of K: the rental list's own level and stats, unlike K's record.
  auto rentalK = card(K, 0, 9, 14, 0, aiK, {33}, 173, 0, 145);
  rentalK->getJSONArray("parameters")->put(14, 85);
  rentalK->put("level_info", Object::parse(R"({"level":30,"level_limit":30,"customize_point":0})"));
  asset(root, "ZIRU_ML_RENTAL_LIST.json", newObj()->put("rental_player_list", newArr()->put(newObj()->put("point", 10)
      ->put("player_info", card(R, 0, 5, 20, 28, aiS, {44}, 61, 0, 145)))->put(newObj()->put("point", 20)->put("player_info", rentalK))));
  asset(root, "CmdCreateUser.json", obj(R"({"user_code":"offline"})"));
  asset(root, "ZIRU_MATCH_TEAMS.json", obj(R"({"teams":{"108":{"base_team_id":108,"team_name":"Test","team_name_short":"TST","kits":[{"id":1,"shirt_rgb":[0,0,0]},{"id":2,"shirt_rgb":[255,255,255]}]}}})"));
  asset(root, "ZIRU_MATCH_PROFILES.json", newObj()->put("profiles", newArr()->put(obj(R"({"profile_id":1,"coordinates":[],"team_style":0,"base_team_id":108})")
      ->put("player_ids", newArr()->put(C)->put(G)->put(Z)))));
  asset(root, "ZIRU_GET_VSCOM_OPPONENT.json", obj(R"({"squad_data":{"squad_gameplan":{"formation":[],"match_plan_settings":{}}},"user_info":[{}]})"));
  asset(root, "ZIRU_START_GAME.json", obj(R"({"squad_data_home":{"coach":{"coach_id":{"id":999,"serial":999}}},"commentary_info":{}})"));
  // A captured MyLeague start: a squad of other people's cards with captured
  // (here: invented) parameters and no play_style, as in the shipped file.
  const long long Y = 4242 + (1LL << 40);  // known only to the template
  auto templatePlayer = [&](long long id, int style1, int style2) {
    auto c = card(id, 900000 + (id & 0xFF), 2, style1, style2, all, {0}, 0, 0, 145);
    return newObj()->put("gameplayer_id", c->getJSONObject("gameplayer_id"))->put("parameters", c->getJSONArray("parameters"))->put("averaged_overall", 0);
  };
  asset(root, "ZIRU_ML_START_GAME.json", newObj()->put("squad_data_home", newObj()->put("team_id", 1)
      ->put("player_list", newArr()->put(templatePlayer(A, 3, 26))->put(templatePlayer(C, 9, 30))->put(templatePlayer(Y, 11, 29)))
      ->put("coach", newObj()->put("coach_id", newObj()->put("id", 999)->put("serial", 999)))->put("squad_gameplan", newObj()))
      ->put("commentary_info", newObj()));
  // A static list served as is: a known card with stale styles, unknown cards whose
  // parameters 61/62 and play_style disagree or are missing, and a card with two attack styles.
  const long long U = 501 + (3LL << 40), V = 502 + (3LL << 40), W = 503 + (3LL << 40), Q = 504 + (3LL << 40);
  auto stale = card(A, 0, 12, 3, 26, all, {29}, 0, 0, 149);
  auto mismatched = card(U, 0, 12, 5, 9, none, {}, 0, 0, 149);
  mismatched->getJSONArray("parameters")->put(61, 0)->put(62, 0);
  auto noStyle = card(V, 0, 12, 22, 27, none, {}, 0, 0, 149);
  noStyle->remove("play_style");
  auto twoAttack = card(W, 0, 12, 14, 6, none, {}, 0, 0, 149);
  auto squadEntry = templatePlayer(Q, 1, 24);
  auto legacy = newObj()->put("gameplayer_id", newObj()->put("id", S)->put("serial", 0))->put("play_style", 20);
  asset(root, "get_mailbox_info.json", newObj()->put("legacy", legacy)->put("mail_list", newArr()->put(newObj()->put("gameplayer", stale))
      ->put(newObj()->put("gameplayer", mismatched))->put(newObj()->put("gameplayer", noStyle))
      ->put(newObj()->put("gameplayer", twoAttack)))->put("squad_data_home", newObj()->put("player_list", newArr()->put(squadEntry))));
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
  // The detail screen of every listed card shows that card: styles, AI styles and skills, also for a
  // starter card whose serial holds another card (T shares 501 with A) and has no record.
  auto detailOf = [&](long long id, long long serial) {
    return call(s, "CMD_GET_MYCLUB_PLAYER_DATA", newObj()->put("gameplayer_id", newObj()->put("id", id)->put("serial", serial)));
  };
  for (int i = 0; i < entry->length(); ++i) {
    auto shown = entry->getJSONObject(i);
    auto g = shown->getJSONObject("gameplayer_id");
    std::string what = "player detail of card " + std::to_string(g->getLong("id")) + " at serial " + std::to_string(g->getLong("serial"));
    auto values = detailOf(g->getLong("id"), g->getLong("serial"))->optJSONArray("parameters");
    check(values != nullptr, (what + " has the card's parameters").c_str());
    auto shownValues = shown->getJSONArray("parameters");
    for (int k = 61; k < 142; ++k)
      check(values->optInt(k, -1) == shownValues->optInt(k, -2), (what + " equals the listed card at parameter " + std::to_string(k)).c_str());
  }
  // A version of A's player that is not the card saved under 501 shows its own record, never A.
  auto otherVersion = detailOf(A2, 501)->getJSONArray("parameters");
  check(otherVersion->getInt(61) == 11 && otherVersion->getInt(64) == 1 && otherVersion->getInt(kZiruSkillParameters[5]) == 1 &&
        otherVersion->getInt(kZiruSkillParameters[29]) == 0, "player detail never mixes two versions of a player");

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
  for (long long serial : {501, 502, 503, 504, 505}) {
    std::string diff = paramDiff(bySerial(roster, serial), bySerial(home, serial));
    check(diff.empty(), ("match start: the user's player " + std::to_string(serial) + " plays with the card's own parameters, " + diff).c_str());
  }
  // GET_MYCLUB_ENTRY_INFO also lists starter entries whose serial now holds another card.
  check(paramDiff(byId(entry, T), byId(home, T)).empty() && byId(home, T)->getJSONArray("parameters")->getInt(33) == 2,
      "match start: a card listed only by entry info plays with the parameters entry info shows");
  // MyLeague without a special squad: the captured template squad is not sent as the user's players.
  s.ziruMyLeagueSetActive(1);
  auto ml = call(s, "CMD_START_GAME", newObj()->put("game_id", 7));
  auto userSide = ml->getJSONObject("squad_data_home"), aiSide = ml->getJSONObject("squad_data_away");
  check(userSide->getJSONArray("player_list")->length() == 0, "MyLeague: no captured player_list on the user's side");
  auto aiPlayers = aiSide->getJSONArray("player_list");
  auto aiSideTemplate = aiPlayers;
  check(aiPlayers->length() == 3 && sameStyles(byId(aiPlayers, A), 3, 0, aiA) && skillBit(byId(aiPlayers, A), 0) == 0 &&
        sameStyles(byId(aiPlayers, C), 9, 0, none), "MyLeague: the AI template's known cards carry their own identity");
  // MyLeague with a special squad: the user's own cards, a rental card, a card only the template knows,
  // and a card nobody knows (left out).
  auto special = newArr();
  for (auto ref : {std::make_pair(A, 501LL), std::make_pair(S, 503LL), std::make_pair(D, 505LL), std::make_pair(R, 0LL),
                   std::make_pair(Y, 0LL), std::make_pair(424242LL, 0LL)})
    special->put(newObj()->put("gameplayer_id", newObj()->put("id", ref.first)->put("serial", ref.second)));
  s.ziruDb->saveMeta("ziru_myleague_special_squad", newObj()->put("squad_data", newObj()->put("player_list", special)
      ->put("coach", newArr()->put(coachRef(100, 1000))))->toString());
  auto login2 = call(s, "CMD_LOGIN")->getJSONObject("entry_info")->getJSONArray("gameplayer_list");
  s.ziruMyLeagueSetActive(1);
  auto mine = call(s, "CMD_START_GAME", newObj()->put("game_id", 8))->getJSONObject("squad_data_home")->getJSONArray("player_list");
  check(mine->length() == 5, "MyLeague: an unknown card is left out instead of copying another player");
  for (long long serial : {501, 503, 505}) {
    std::string diff = paramDiff(bySerial(login2, serial), bySerial(mine, serial));
    check(diff.empty(), ("MyLeague: the user's player " + std::to_string(serial) + " plays with the card's own parameters, " + diff).c_str());
  }
  check(sameStyles(byId(mine, R), 20, 28, aiS), "MyLeague: a rental card plays with its rental entry");
  check(byId(mine, Y)->getJSONArray("parameters")->getInt(61) == 11, "MyLeague: a card only the template knows uses the template entry");
  // Rental cards play with the cards the rental list shows: the rental's own level and stats (K's
  // rental differs from K's record) and the same normalisation (R has no weak-foot values listed).
  auto rentals = call(s, "CMD_GET_SEASON_POINT_RENTALPLAYER_LIST")->getJSONArray("rental_player_list");
  auto rentalSquad = newArr();
  for (int i = 0; i < rentals->length(); ++i)
    rentalSquad->put(newObj()->put("gameplayer_id", Object::parse(rentals->getJSONObject(i)->getJSONObject("player_info")->getJSONObject("gameplayer_id")->toString())));
  s.ziruDb->saveMeta("ziru_myleague_special_squad", newObj()->put("squad_data", newObj()->put("player_list", rentalSquad)
      ->put("coach", newArr()->put(coachRef(100, 1000))))->toString());
  s.ziruMyLeagueSetActive(1);
  auto rentalPlayers = call(s, "CMD_START_GAME", newObj()->put("game_id", 9))->getJSONObject("squad_data_home")->getJSONArray("player_list");
  check(rentalPlayers->length() == 2, "MyLeague: both rental cards play");
  for (int i = 0; i < rentals->length(); ++i) {
    auto shown = rentals->getJSONObject(i)->getJSONObject("player_info");
    auto played = byId(rentalPlayers, shown->getJSONObject("gameplayer_id")->getLong("id"));
    auto a = shown->getJSONArray("parameters"), b = played->getJSONArray("parameters");
    std::string diff = a->length() == b->length() ? "" : "length";
    for (int k = 0; diff.empty() && k < a->length(); ++k)
      if (k != 59 && k != 60 && a->optInt(k, -1) != b->optInt(k, -1)) diff = "index " + std::to_string(k);
    check(diff.empty(), ("MyLeague: rental " + std::to_string(shown->getJSONObject("gameplayer_id")->getLong("id")) +
                         " plays with the card the rental list shows, " + diff).c_str());
  }
  check(byId(rentalPlayers, K)->getJSONArray("parameters")->getInt(14) == 85, "MyLeague: a rental keeps the rental list's stats, not its record's");
  // A special squad none of whose cards is known still replaces the captured template squad.
  s.ziruDb->saveMeta("ziru_myleague_special_squad", newObj()->put("squad_data", newObj()->put("player_list", newArr()
      ->put(newObj()->put("gameplayer_id", newObj()->put("id", 424242LL)->put("serial", 0)))))->toString());
  s.ziruMyLeagueSetActive(1);
  auto emptySide = call(s, "CMD_START_GAME", newObj()->put("game_id", 10))->getJSONObject("squad_data_home")->getJSONArray("player_list");
  check(emptySide->length() == 0, "MyLeague: a special squad with no known card sends an empty player_list, not the captured one");
  s.ziruMyLeagueSetActive(0);
  s.ziruDb->saveMeta("ziru_myleague_special_squad", "");
  check(sqlite3_exec(s.ziruDb->db, "DELETE FROM meta WHERE k='ziru_myleague_special_squad'", nullptr, nullptr, nullptr) == SQLITE_OK, "clear special squad");


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
    // E2 has no record: 75288ee filled its empty booster slot 1 from E, the first record of the same
    // player (136); slot 0 kept E2's own booster 12 and the player later trained 77 into slot 2.
    auto e2 = card(E2, 509, 6, 20, 0, none, {3}, 108, 0, 145);
    e2->put("initial_booster_id_list", newArr()->put(12)->put(136)->put(77));
    e2->getJSONArray("parameters")->put(142, 12)->put(143, 136)->put(144, 77);
    e2->put("original_parameters", Array::parse(e2->getJSONArray("parameters")->toString()));
    srv->ziruDb->savePlayer(509, E2, e2->toString());
    // A device that already ran the first version of this repair still gets the booster step.
    srv->ziruDb->saveMeta("z27_card_identity_v1", "1");
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
    auto boosters = [](const ObjP& c, const char* key) {
      auto v = c->getJSONArray(key);
      return std::to_string(v->getInt(142)) + "," + std::to_string(v->getInt(143)) + "," + std::to_string(v->getInt(144));
    };
    auto e2 = bySerial(list, 509), savedE2 = Object::parse(*srv->loadPlayer(509));
    check(e2->getJSONArray("initial_booster_id_list")->toString() == "[12,0,77]" && boosters(e2, "parameters") == "12,0,77",
        "old save: a booster borrowed from another version of the player is removed; its own and the trained one stay");
    check(savedE2->getJSONArray("initial_booster_id_list")->toString() == "[12,0,77]" && boosters(savedE2, "parameters") == "12,0,77" &&
          boosters(savedE2, "original_parameters") == "12,0,77", "old save: the corrected boosters are saved");
    check(boosters(e, "parameters") == "186,136,0", "old save: a card with its own record keeps its boosters");
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
  // A special or Dummy skill cannot be selected, and no other skill is taught in its place:
  // the training is not successful and the card stays as it is.
  for (int refused : {29, 9, 72, 73}) {
    std::string row = *s.loadPlayer(501), learned = s.sp("almanac_state").getString("ziru_skills_501", "");
    auto req = newObj()->put("gameplayer_id_list", newArr()->put(newObj()->put("id", A)->put("serial", 501)))->put("training_index", 0)
        ->put("skill_id", refused)->put("item_list", newArr()->put(newObj()->put("present_num", 1)->put("item", newObj()->put("item_type", "SKILL_TRAINING_SELECT"))));
    auto r = call(s, "CMD_USE_MYCLUB_ITEM", req)->getJSONObject("skill_training_result");
    check(r->getString("is_success") == "NO" && !r->has("additional_skill_id") && *s.loadPlayer(501) == row &&
          s.sp("almanac_state").getString("ziru_skills_501", "") == learned, "a special or Dummy skill cannot be selected, and nothing else is taught");
  }
  check(train("SKILL_TRAINING_SELECT", A, 501, newObj()->put("skill_id", 44)) == 44, "a normal skill can be selected");
  auto trainedA = Object::parse(*s.loadPlayer(501));
  check(skillCount(trainedA) == 2 && skillBit(trainedA, 29) == 1 && skillBit(trainedA, 44) == 1,
      "retraining one slot leaves the card's own skill plus the one learned skill");
  check(sameStyles(trainedA, 3, 0, aiA), "training keeps the card's styles");
  // Learned skills belong to the card saved under the serial. T shares serial 501 with A in the
  // starter data: training A must not change how T plays.
  {
    auto opp = call(s, "CMD_GET_VSCOM_OPPONENT");
    auto after = call(s, "CMD_START_GAME", newObj()->put("game_id", (long long)opp->getLong("game_id")))
        ->getJSONObject("squad_data_home")->getJSONArray("player_list");
    std::string diff = paramDiff(byId(home, T), byId(after, T));
    check(diff.empty(), ("training A leaves T, which shares A's serial, as it was: " + diff).c_str());
    check(skillBit(byId(after, A), 44) == 1, "the trained card plays with its learned skill");
  }
  // A category with nothing left for the card: any other normal skill instead of an error.
  auto useItem = [&](OfflineServer& srv, const char* type, long long id, long long serial, int index, const char* category) {
    auto req = newObj()->put("gameplayer_id_list", newArr()->put(newObj()->put("id", id)->put("serial", serial)))->put("training_index", index)
        ->put("item_list", newArr()->put(newObj()->put("present_num", 1)->put("item", newObj()->put("item_type", type))));
    if (category) req->put("skill_training_category_type", category);
    return call(srv, "CMD_USE_MYCLUB_ITEM", req);
  };
  const std::vector<int> defenceNormal = {55, 56, 57, 58, 59, 60, 64};
  auto savedX = card(X, 506, 9, 9, 0, none, {55, 56, 57, 58, 59, 60, 64}, 108, 0, 145);
  savedX->put("additional_skill_id", newArr()->put(57)->put(58)->put(59)->put(60)->put(64));
  s.ziruDb->savePlayer(506, X, savedX->toString());
  s.sp("almanac_state").edit().putString("ziru_skills_506", "57,58,59,60,64").commit();
  auto defence = useItem(s, "SKILL_TRAINING_CATEGORY", X, 506, 0, "DEFENCE")->getJSONObject("skill_training_result");
  int got = defence->getInt("additional_skill_id");
  check(defence->getString("is_success") == "YES" && isZiruTrainableSkill(got) && std::find(defenceNormal.begin(), defenceNormal.end(), got) == defenceNormal.end(),
      "a category with nothing left teaches another normal skill");
  check(s.sp("almanac_state").getString("ziru_skills_506", "") == std::to_string(got) + ",58,59,60,64", "the new skill takes the trained slot");
  // Nothing left at all: retraining a slot keeps its skill; a new slot is refused and nothing changes.
  auto savedFull1 = card(FULL1, 507, 9, 9, 0, none, allNormal, 108, 0, 145);
  savedFull1->put("additional_skill_id", newArr()->put(0)->put(1)->put(2)->put(3)->put(4));
  s.ziruDb->savePlayer(507, FULL1, savedFull1->toString());
  s.sp("almanac_state").edit().putString("ziru_skills_507", "0,1,2,3,4").commit();
  auto kept = useItem(s, "SKILL_TRAINING", FULL1, 507, 2, nullptr)->getJSONObject("skill_training_result");
  check(kept->getString("is_success") == "YES" && kept->getInt("additional_skill_id") == 2 &&
        s.sp("almanac_state").getString("ziru_skills_507", "") == "0,1,2,3,4", "nothing left: the retrained slot keeps its skill");
  s.ziruDb->savePlayer(508, FULL2, card(FULL2, 508, 9, 9, 0, none, allNormal, 108, 0, 145)->toString());
  std::string before = *s.loadPlayer(508);
  auto none2 = useItem(s, "SKILL_TRAINING_CATEGORY", FULL2, 508, 0, "SHOOT");
  auto refused = none2->getJSONObject("skill_training_result");
  check(refused->getString("is_success") == "NO" && !refused->has("additional_skill_id") && *s.loadPlayer(508) == before &&
        s.sp("almanac_state").getString("ziru_skills_508", "").empty(), "nothing left for a new slot: not successful, the card is unchanged");
  bool itemKept = false;
  auto items = none2->getJSONArray("item_list");
  for (int i = 0; items && i < items->length(); ++i) {
    auto it = items->getJSONObject(i);
    if (it->getJSONObject("item")->optString("item_type") == "SKILL_TRAINING_CATEGORY") itemKept = it->getInt("inventory_num") == 999999;
  }
  check(itemKept, "the training item is not used up");
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
  {
    // The 11.1.1 client sends the skill picked in Skill Training (Select) as ability_id and as the
    // item's effect, in its own skill numbers; booster training sends a booster id as ability_id.
    Config wireCfg = cfg; wireCfg.dataDir = root + "/select-ios-train"; wireCfg.iosPlayerLayout = true;
    std::filesystem::create_directories(wireCfg.dataDir);
    OfflineServer w(wireCfg);
    call(w, "CMD_LOGIN");
    auto pick = [&](const char* type, int index, int ability) {
      auto req = newObj()->put("gameplayer_id_list", newArr()->put(newObj()->put("id", A)->put("serial", 501)))->put("training_index", index)
          ->put("ability_id", ability)->put("position_id", 0)
          ->put("item_list", newArr()->put(newObj()->put("present_num", 100)
              ->put("item", newObj()->put("item_type", type)->put("grade", 0)->put("effect", std::string(type) == "SKILL_TRAINING_SELECT" ? ability : 0))));
      return call(w, "CMD_USE_MYCLUB_ITEM", req);
    };
    int first = pick("SKILL_TRAINING_SELECT", 0, iosSkillId(44, true))->getJSONObject("skill_training_result")->getInt("additional_skill_id");
    int second = pick("SKILL_TRAINING_SELECT", 1, iosSkillId(69, true))->getJSONObject("skill_training_result")->getInt("additional_skill_id");
    check(first == iosSkillId(44, true) && second == iosSkillId(69, true), "iOS Select: the client's skill numbers are answered in the client's numbers");
    check(w.sp("almanac_state").getString("ziru_skills_501", "") == "44,69", "iOS Select: the picked skills are the ones learned");
    auto learnedA = Object::parse(*w.loadPlayer(501));
    check(skillBit(learnedA, 44) == 1 && skillBit(learnedA, 69) == 1 && skillBit(learnedA, 45) == 0 && skillBit(learnedA, 70) == 0,
        "iOS Select: the picked skills' flags are on, the neighbours' are not");
    pick("ABILITY_TRAINING_SELECT", 1, 186);
    check(Object::parse(*w.loadPlayer(501))->getJSONArray("initial_booster_id_list")->getInt(1) == 186, "iOS booster Select: the booster id is used as sent");
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
  // Last check on every response: each card leaves with its own styles.
  for (bool ios : {false, true}) {
    Config netCfg = cfg; netCfg.dataDir = root + (ios ? "/net-ios" : "/net"); netCfg.iosPlayerLayout = ios;
    std::filesystem::create_directories(netCfg.dataDir);
    OfflineServer n(netCfg);
    auto mail = call(n, "CMD_GET_MAILBOX_INFO");
    auto list = mail->getJSONArray("mail_list");
    auto at = [&](int i) { return list->getJSONObject(i)->getJSONObject("gameplayer"); };
    check(sameStyles(at(0), 3, 0, aiA), "outgoing: a known card in a static list gets its own styles and AI styles");
    check(sameStyles(at(1), 5, 9, none), "outgoing: parameters 61/62 follow play_style");
    check(sameStyles(at(2), 22, 27, none), "outgoing: a card without play_style gets it from parameters 61/62");
    check(sameStyles(mail->getJSONObject("squad_data_home")->getJSONArray("player_list")->getJSONObject(0), 1, 24, all),
        "outgoing: squad player_list entries get play_style from their parameters");
    check(sameStyles(at(3), 14, 6, none) && n.ziruStyleRuleLogged.count(W) == 1 && n.ziruStyleRuleLogged.count(U) == 0,
        "outgoing: two styles on one side are sent as printed and reported");
    if (ios) check(at(0)->getJSONArray("parameters")->length() == 147, "outgoing: the check runs before the iOS translation");
    auto legacyStyles = mail->getJSONObject("legacy")->optJSONArray("play_style");
    check(legacyStyles && legacyStyles->getInt(0) == 14 && legacyStyles->getInt(1) == 0,
        "outgoing: a single-number play_style of a known card is replaced by its two styles");
  }
  // CmdLogin.json is read once per process. A login parses it once more (it is the
  // response itself); entry info, match start, player detail and training do not parse it.
  {
    auto parses = [&](const char* cmd, ObjP req) { s.ziruLoginParses = 0; call(s, cmd, req); return s.ziruLoginParses; };
    check(parses("CMD_LOGIN", newObj()) <= 1, "parses: a login reads CmdLogin.json at most once");
    check(parses("CMD_GET_MYCLUB_ENTRY_INFO", newObj()) == 0, "parses: entry info uses the starter cards read before");
    auto game = call(s, "CMD_GET_VSCOM_OPPONENT")->getLong("game_id");
    check(parses("CMD_START_GAME", newObj()->put("game_id", (long long)game)) == 0, "parses: match start uses the starter cards read before");
    check(parses("CMD_GET_MYCLUB_PLAYER_DATA", newObj()->put("gameplayer_id", newObj()->put("id", T)->put("serial", 501))) == 0,
        "parses: player detail uses the starter cards read before");
    auto booster = newObj()->put("gameplayer_id_list", newArr()->put(newObj()->put("id", S)->put("serial", 503)))->put("training_index", 2)
        ->put("ability_id", 77)->put("item_list", newArr()->put(newObj()->put("present_num", 1)->put("item", newObj()->put("item_type", "ABILITY_TRAINING_SELECT"))));
    check(parses("CMD_USE_MYCLUB_ITEM", booster) == 0, "parses: training uses the starter cards read before");
  }
  auto aiY = byId(aiSideTemplate, Y);
  check(sameStyles(aiY, 11, 29, all), "outgoing: an AI template card nobody knows keeps its parameters and gets a matching play_style");
  auto rental = s.z27Player(newObj()->put("id", R)->put("serial", 0LL));
  check(rental && sameStyles(rental, 20, 28, aiS) && skillBit(rental, 44) == 1 && rental->getInt("initial_base_team_id") == 61,
      "a MyLeague rental card uses its own rental entry");
}

// ---- One card per serial -----------------------------------------------------
// Every card the user owns has its own serial, as in Konami's data. Everything
// saved under a serial (the row, careerplan points, learned skills, the sold
// flag) belongs to the card saved there and to no other card.
static std::string fileBytes(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}
static int playerWrites = 0;
static void countPlayerWrites(void*, int, const char*, const char* table, sqlite3_int64) {
  if (std::string(table) == "players") playerWrites++;
}

static void serialTests(const std::string& top) {
  const std::string root = top + "/serials";
  std::filesystem::create_directories(root + "/responses");
  const std::vector<int> none = {0, 0, 0, 0, 0, 0, 0}, aiQ = {0, 0, 0, 1, 0, 0, 0};
  const long long P1 = 14 + (1LL << 40);   // record; the card saved at 801
  const long long P2 = 14 + (2LL << 40);   // another version of the same player, record only
  const long long Q = 14 + (3LL << 40);    // starter entry only, at 802; a third version of that player
  const long long G = 30 + (1LL << 40);    // record; at 805
  const long long H = 31 + (1LL << 40);    // record; at 807, sold below
  const long long S1 = 40 + (1LL << 40), S2 = 41 + (1LL << 40);  // two cards listed under one serial (810), as build 138 shipped them
  const long long Z = 40002;               // known only to the start-game template
  const long long U = 40352, U2 = 89133993205152LL;    // the iOS client knows starter card 40352 as U2 (it has a record)
  const long long K0 = 44379, K = 106788187843931LL;   // and 44379 as K (it has a record)
  auto records = newObj();
  for (auto c : {card(P1, 0, 12, 3, 0, none, {29}, 108, 0, 149), card(P2, 0, 12, 11, 0, none, {5}, 108, 0, 149),
                 card(G, 0, 9, 14, 0, none, {33}, 108, 0, 149), card(H, 0, 9, 14, 0, none, {33}, 108, 0, 149),
                 card(S1, 0, 9, 1, 0, none, {2}, 108, 0, 149), card(S2, 0, 9, 2, 0, none, {3}, 108, 0, 149)})
    records->put(std::to_string(c->getJSONObject("gameplayer_id")->getLong("id")), newObj()->put("gameplayer", c));
  for (long long id : {U2, K}) {  // the records of the upgraded ids differ from the starter entries
    auto rec = card(id, 0, 1, 14, 0, none, {33}, 0, 0, 149);
    rec->getJSONArray("parameters")->put(14, 99)->put(20, 99);
    rec->put("level_info", Object::parse(R"({"level":1,"level_limit":32,"customize_point":0})"));
    records->put(std::to_string(id), newObj()->put("gameplayer", rec));
  }
  asset(root, "ZIRU_PLAYER_RECORDS.json", records);
  auto starters = newArr(), refs = newArr();
  auto list = [&](const ObjP& c) {
    starters->put(c);
    refs->put(newObj()->put("gameplayer_id", Object::parse(c->getJSONObject("gameplayer_id")->toString()))->put("uniform_number", refs->length() + 1));
  };
  list(card(P1, 801, 12, 3, 0, none, {29}, 108, 0, 145));
  list(card(Q, 802, 12, 22, 0, aiQ, {7}, 108, 0, 145));
  list(card(K0, 803, 1, 14, 0, none, {33}, 0, 0, 145));
  list(card(G, 805, 9, 14, 0, none, {33}, 108, 0, 145));
  list(card(H, 807, 9, 14, 0, none, {33}, 108, 0, 145));
  list(card(S1, 810, 9, 1, 0, none, {2}, 108, 0, 145));
  list(card(S2, 810, 9, 2, 0, none, {3}, 108, 0, 145));
  list(card(U, 810, 1, 14, 0, none, {33}, 0, 0, 145));
  refs->put(newObj()->put("gameplayer_id", newObj()->put("id", Z)->put("serial", 820))->put("uniform_number", 23));
  auto squad = newObj()->put("player_list", refs)->put("coach", newArr()->put(coachRef(100, 1000)))->put("squad_gameplan", newObj());
  asset(root, "CmdLogin.json", newObj()->put("entry_info", newObj()->put("main_squad_id", 9)->put("gameplayer_list", starters)
      ->put("coach_list", newArr()->put(coachRef(100, 1000)))
      ->put("squad_list", newArr()->put(newObj()->put("squad_id", 9)->put("squad_data", squad)
          ->put("base_team", newArr()->put(newObj()->put("base_team_id", 108)))))));
  asset(root, "CmdCreateUser.json", obj(R"({"user_code":"offline"})"));
  asset(root, "ZIRU_MATCH_TEAMS.json", obj(R"({"teams":{"108":{"base_team_id":108,"team_name":"Test","team_name_short":"TST","kits":[{"id":1,"shirt_rgb":[0,0,0]},{"id":2,"shirt_rgb":[255,255,255]}]}}})"));
  asset(root, "ZIRU_MATCH_PROFILES.json", obj(R"({"profiles":[{"profile_id":1,"player_ids":[],"coordinates":[],"team_style":0,"base_team_id":108}]})"));
  asset(root, "ZIRU_GET_VSCOM_OPPONENT.json", obj(R"({"squad_data":{"squad_gameplan":{"formation":[],"match_plan_settings":{}}},"user_info":[{}]})"));
  auto templateZ = card(Z, 820, 2, 1, 0, none, {0}, 0, 0, 145);
  asset(root, "ZIRU_START_GAME.json", newObj()->put("squad_data_home", newObj()->put("coach", newObj()->put("coach_id", newObj()->put("id", 999)->put("serial", 999)))
      ->put("player_list", newArr()->put(newObj()->put("gameplayer_id", templateZ->getJSONObject("gameplayer_id"))
          ->put("parameters", templateZ->getJSONArray("parameters"))->put("averaged_overall", 0))))->put("commentary_info", newObj()));
  // Konami's item answer carries a successful skill result of its own.
  asset(root, "CMD_USE_MYCLUB_ITEM.json", obj(R"({"skill_training_result":{"additional_skill_id":22,"is_success":"YES"},"item_list":[{"item":{"item_type":"SKILL_TRAINING","grade":0,"effect":0},"inventory_num":5}],"result":"NOERR"})"));
  asset(root, "ZIRU_RANDOM_BOOSTERS.json", obj(R"({"ability_ids":[12,20,30]})"));
  asset(root, "generic.json", obj(R"({"result":"NOERR"})"));
  Config cfg; cfg.assetsDir = root; cfg.dataDir = root + "/save";
  std::filesystem::create_directories(cfg.dataDir);
  OfflineServer s(cfg);
  auto rowText = [&](long long serial) { JStr row = s.loadPlayer(serial); return row ? *row : std::string(); };
  auto listed = [](const ObjP& r) { return r->getJSONObject("entry_info")->getJSONArray("gameplayer_list"); };

  // Serial 802 holds P2, another version of Q's player, with P2's learned skill
  // and careerplan points (as a build that shared serials could leave it).
  auto savedP2 = card(P2, 802, 12, 11, 0, none, {5, 44}, 108, 0, 145);
  savedP2->put("additional_skill_id", newArr()->put(44));
  s.ziruDb->savePlayer(802, P2, savedP2->toString());
  s.ziruDb->saveProgression(802, R"({"SHOOT":10,"_spent_total":10})");
  s.sp("almanac_state").edit().putString("ziru_skills_802", "44").commit();
  std::string p2Row = rowText(802);
  call(s, "CMD_CREATE_USER");
  for (const char* cmd : {"CMD_LOGIN", "CMD_GET_MYCLUB_ENTRY_INFO"}) {
    auto shown = bySerial(listed(call(s, cmd)), 802);
    std::string what = std::string(cmd) + ": ";
    check(shown->getJSONObject("gameplayer_id")->getLong("id") == Q && sameStyles(shown, 22, 0, aiQ),
        (what + "the listed card is sent as itself, not the other version saved under its serial").c_str());
    check(shown->getJSONArray("additional_skill_id")->length() == 0 && skillBit(shown, 44) == 0,
        (what + "the listed card does not get the skill learned by the other card").c_str());
    if (std::string(cmd) == "CMD_LOGIN")
      check(shown->getJSONObject("level_info")->getInt("customize_point") == 60, "login: the listed card does not get the other card's careerplan points");
    check(rowText(802) == p2Row, (what + "the other card saved under the serial is left as it is").c_str());
  }
  s.refreshZiruGameplayerFromCanonical(card(Q, 802, 12, 22, 0, aiQ, {7}, 108, 0, 145));
  check(rowText(802) == p2Row, "refresh: the other card saved under the serial is left as it is");
  auto qDetail = call(s, "CMD_GET_MYCLUB_PLAYER_DATA", newObj()->put("gameplayer_id", newObj()->put("id", Q)->put("serial", 802)));
  auto plan = qDetail->getJSONArray("careerplan_list")->getJSONObject(0)->getJSONArray("data_list")->getJSONObject(0);
  check(plan->getString("parameter_type") == "SHOOT" && plan->getInt("count") == 0, "player detail: careerplan points of another card are not shown");
  auto p2Detail = call(s, "CMD_GET_MYCLUB_PLAYER_DATA", newObj()->put("gameplayer_id", newObj()->put("id", P2)->put("serial", 802)));
  check(p2Detail->getJSONArray("careerplan_list")->getJSONObject(0)->getJSONArray("data_list")->getJSONObject(0)->getInt("count") == s.ziruCountFromSpent(10),
      "player detail: the saved card shows its own careerplan points");
  // Two cards listed under one serial (810): the login keeps the first one saved there, and
  // listing the second one does not write it over the first one's row.
  sqlite3_update_hook(s.ziruDb->db, countPlayerWrites, nullptr);
  playerWrites = 0;
  call(s, "CMD_LOGIN");
  call(s, "CMD_GET_MYCLUB_ENTRY_INFO");
  check(playerWrites == 0, ("a login and entry info on an unchanged save write no player row, wrote " + std::to_string(playerWrites)).c_str());
  sqlite3_update_hook(s.ziruDb->db, nullptr, nullptr);
  check(Object::parse(rowText(810))->getJSONObject("gameplayer_id")->getLong("id") == S1, "the first card listed under a shared serial keeps its row");

  // ---- Starter cards the client knows by an upgraded id; learned skills of template cards ----
  {
    auto entryList = listed(call(s, "CMD_GET_MYCLUB_ENTRY_INFO"));
    auto loginList = listed(call(s, "CMD_LOGIN"));
    auto shownU = byId(entryList, U2), shownK = byId(loginList, K);
    check(shownU->getJSONObject("gameplayer_id")->getLong("serial") == 810 && shownK->getJSONObject("gameplayer_id")->getLong("serial") == 803,
        "the upgraded starter cards are listed under their own ids");
    // Learned skills saved for serial 820 belong to no card the user owns there.
    s.sp("almanac_state").edit().putString("ziru_skills_820", "44").commit();
    auto game = call(s, "CMD_GET_VSCOM_OPPONENT")->getLong("game_id");
    auto played = call(s, "CMD_START_GAME", newObj()->put("game_id", (long long)game))->getJSONObject("squad_data_home")->getJSONArray("player_list");
    auto playedZ = byId(played, Z);
    check(!playedZ->has("additional_skill_id") && skillBit(playedZ, 44) == 0,
        "match start: a card known only to the template gets no learned skills from its serial");
    s.sp("almanac_state").edit().remove("ziru_skills_820").commit();
    // U2's serial holds S1: the card is not saved, so the match and the detail screen use its starter entry, as listed.
    std::string diff = paramDiff(shownU, byId(played, U2));
    check(diff.empty(), ("match start: an upgraded starter card plays as listed, not as the record of its new id, " + diff).c_str());
    auto detailU = call(s, "CMD_GET_MYCLUB_PLAYER_DATA", newObj()->put("gameplayer_id", newObj()->put("id", U2)->put("serial", 810)));
    check(detailU->getJSONArray("parameters")->toString() == shownU->getJSONArray("parameters")->toString(),
        "player detail: an upgraded starter card shows the listed card, not the record of its new id");
    // K is saved under its own serial: match and detail use the saved card, level included.
    diff = paramDiff(shownK, byId(played, K));
    check(diff.empty(), ("match start: a saved upgraded card plays as the login shows it, " + diff).c_str());
    auto detailK = call(s, "CMD_GET_MYCLUB_PLAYER_DATA", newObj()->put("gameplayer_id", newObj()->put("id", K)->put("serial", 803)));
    check(detailK->getJSONArray("parameters")->toString() == shownK->getJSONArray("parameters")->toString() &&
          s.z27Player(newObj()->put("id", K)->put("serial", 803))->getJSONObject("level_info")->getInt("level") ==
              shownK->getJSONObject("level_info")->getInt("level"), "player detail and match: a saved upgraded card as the login shows it");
  }

  // ---- Training changes exactly the card the client picked ----
  // Everything saved: player rows, careerplan points and prefs, by key.
  auto saveState = [&]() {
    std::map<std::string, std::string> out;
    for (auto& [serial, text] : s.ziruDb->loadAllPlayerRows()) out["player " + std::to_string(serial)] = text;
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(s.ziruDb->db, "SELECT serial, progression_json FROM progression", -1, &st, nullptr);
    while (sqlite3_step(st) == SQLITE_ROW) out["progression " + std::to_string(sqlite3_column_int64(st, 0))] = (const char*)sqlite3_column_text(st, 1);
    sqlite3_finalize(st);
    auto& prefs = s.sp("almanac_state");
    for (const auto& k : prefs.data->keys()) out["prefs " + k] = prefs.data->optJSONObject(k)->toString();
    return out;
  };
  auto changedKeys = [](const std::map<std::string, std::string>& a, const std::map<std::string, std::string>& b) {
    std::set<std::string> keys;
    for (auto& kv : a) if (!b.count(kv.first) || b.at(kv.first) != kv.second) keys.insert(kv.first);
    for (auto& kv : b) if (!a.count(kv.first)) keys.insert(kv.first);
    return keys;
  };
  auto item = [&](const char* type, long long id, long long serial, int index, ObjP extra) {
    auto req = newObj()->put("gameplayer_id_list", newArr()->put(newObj()->put("id", id)->put("serial", serial)))->put("training_index", index)
        ->put("item_list", newArr()->put(newObj()->put("present_num", 1)->put("item", newObj()->put("item_type", type))));
    for (const auto& k : extra->keys()) req->put(k, extra->get(k));
    return call(s, "CMD_USE_MYCLUB_ITEM", req);
  };
  auto career = [&](long long id, long long serial, int shoot) {
    auto use = newArr();
    if (shoot > 0) use->put(newObj()->put("parameter_type", "SHOOT")->put("point", shoot));
    return call(s, "CMD_USE_MYCLUB_CAREERPLAN", newObj()->put("gameplayer_use_list", newArr()->put(
        newObj()->put("gameplayer_id", newObj()->put("id", id)->put("serial", serial))->put("use_list", use))));
  };
  struct Training { const char* type; std::function<ObjP(long long, long long)> run; };
  std::vector<Training> trainings = {
    {"SKILL_TRAINING_SELECT", [&](long long id, long long serial) { return item("SKILL_TRAINING_SELECT", id, serial, 0, newObj()->put("skill_id", 44)); }},
    {"SKILL_TRAINING", [&](long long id, long long serial) { return item("SKILL_TRAINING", id, serial, 1, newObj()); }},
    {"SKILL_TRAINING_CATEGORY", [&](long long id, long long serial) { return item("SKILL_TRAINING_CATEGORY", id, serial, 2, newObj()->put("skill_training_category_type", "PASS")); }},
    {"ABILITY_TRAINING_SELECT", [&](long long id, long long serial) { return item("ABILITY_TRAINING_SELECT", id, serial, 0, newObj()->put("ability_id", 186)); }},
    {"ABILITY_TRAINING_RANDOM", [&](long long id, long long serial) { return item("ABILITY_TRAINING_RANDOM", id, serial, 1, newObj()); }},
    {"POSITION_TRAINING", [&](long long id, long long serial) { return item("POSITION_TRAINING", id, serial, 0, newObj()); }},
    // ST is next to G's own LWG, so the client offers it for slot 1 whatever the random training taught in slot 0.
    {"POSITION_TRAINING_SELECT", [&](long long id, long long serial) { return item("POSITION_TRAINING_SELECT", id, serial, 1, newObj()->put("position_id", "ST")); }},
    {"CAREERPLAN", [&](long long id, long long serial) { return career(id, serial, 6); }},
    {"CAREERPLAN reset", [&](long long id, long long serial) { return career(id, serial, 0); }},
  };
  // G has its own serial: each training changes G and nothing else.
  auto loginBefore = listed(call(s, "CMD_LOGIN"));
  for (auto& t : trainings) {
    auto before = saveState();
    auto r = t.run(G, 805);
    std::set<std::string> mine = {"player 805", "progression 805", "prefs ziru_skills_805"};
    for (const auto& k : changedKeys(before, saveState()))
      check(mine.count(k) == 1, (std::string(t.type) + " on G changed " + k + ", which is not G's").c_str());
    check(r->toString().find("\"is_success\":\"NO\"") == std::string::npos, (std::string(t.type) + " on G is not refused").c_str());
  }
  auto savedG = Object::parse(rowText(805));
  check(savedG->getJSONArray("initial_booster_id_list")->toString() != "[0,0,0]" && savedG->optJSONArray("additional_position") &&
        skillBit(savedG, 44) == 1 && s.loadProgression(805), "G keeps what it was trained: booster, position, skill, careerplan points");
  // The login shows the trained G and every other card as before; the match plays every card as shown.
  auto loginAfter = listed(call(s, "CMD_LOGIN"));
  check(loginAfter->length() == loginBefore->length(), "training adds or removes no card");
  for (int i = 0; i < loginAfter->length(); ++i) {
    auto card = loginAfter->getJSONObject(i);
    auto gid = card->getJSONObject("gameplayer_id");
    auto was = bySerial(loginBefore, gid->getLong("serial"));
    bool trained = gid->getLong("serial") == 805;
    check((card->toString() == was->toString()) != trained,
        ("login after training: card " + std::to_string(gid->getLong("id")) + (trained ? " shows its training" : " is unchanged")).c_str());
  }
  check(skillBit(bySerial(loginAfter, 805), 44) == 1, "login after training: G shows its learned skill");
  auto game = call(s, "CMD_GET_VSCOM_OPPONENT")->getLong("game_id");
  auto played = call(s, "CMD_START_GAME", newObj()->put("game_id", (long long)game))->getJSONObject("squad_data_home")->getJSONArray("player_list");
  for (long long serial : {801, 802, 805, 807, 810}) {
    std::string diff = paramDiff(bySerial(loginAfter, serial), bySerial(played, serial));
    check(diff.empty(), ("match start: the card at " + std::to_string(serial) + " plays as the login shows it, " + diff).c_str());
  }
  auto gDetail = call(s, "CMD_GET_MYCLUB_PLAYER_DATA", newObj()->put("gameplayer_id", newObj()->put("id", G)->put("serial", 805)));
  check(gDetail->getJSONArray("parameters")->toString() == bySerial(loginAfter, 805)->getJSONArray("parameters")->toString(),
      "player detail: G shows the trained card the login shows");
  // Requests that name a card the serial does not hold are refused and change nothing.
  call(s, "CMD_SELL_MYCLUB_GAMEPLAYER", newObj()->put("gameplayer_id_list", newArr()->put(newObj()->put("id", H)->put("serial", 807))));
  struct Probe { const char* what; long long id, serial; };
  for (Probe probe : {Probe{"a second card listed under a serial that holds the first one", S2, 810},
                      Probe{"a card under another card's serial", G, 801}, Probe{"a card under serial 0", G, 0},
                      Probe{"a sold card", H, 807}, Probe{"an unknown card under a card's serial", 990000000001LL, 805},
                      Probe{"a card under a serial nothing is saved or listed under", G, 811}}) {
    for (auto& t : trainings) {
      std::string what = std::string(t.type) + " on " + probe.what;
      std::string db = fileBytes(cfg.dataDir + "/ziru_save.db"), prefs = fileBytes(cfg.dataDir + "/prefs_almanac_state.json");
      auto r = t.run(probe.id, probe.serial);
      check(fileBytes(cfg.dataDir + "/ziru_save.db") == db && fileBytes(cfg.dataDir + "/prefs_almanac_state.json") == prefs,
          (what + ": the save is not changed").c_str());
      check(r->toString().find("\"is_success\":\"YES\"") == std::string::npos, (what + ": no successful result is sent").c_str());
      if (std::string(t.type).rfind("CAREERPLAN", 0) == 0) {
        auto results = r->getJSONArray("careerplan_result_list");
        check(results->length() <= 1 && (results->length() == 0 || (results->getJSONObject(0)->getJSONArray("careerplan_list")->length() == 0 &&
              results->getJSONObject(0)->getJSONObject("gameplayer")->getJSONObject("gameplayer_id")->getLong("id") == probe.id)),
            (what + ": the answer shows the card unchanged").c_str());
        continue;
      }
      const char* key = std::string(t.type).rfind("SKILL", 0) == 0 ? "skill_training_result"
                      : std::string(t.type).rfind("ABILITY", 0) == 0 ? "ability_training_result" : "position_training_result";
      auto result = r->optJSONObject(key);
      check(result && result->getString("is_success") == "NO", (what + ": the training is answered as not successful").c_str());
      for (const char* other : {"skill_training_result", "ability_training_result", "position_training_result"})
        check(std::string(other) == key || !r->has(other), (what + ": only this training's result is sent").c_str());
      auto items = r->getJSONArray("item_list");
      for (int i = 0; i < items->length(); ++i)
        check(items->getJSONObject(i)->getInt("inventory_num") == 999999, (what + ": no item is used up").c_str());
    }
  }
}

// ---- Saves written while the starter cards shared 11 serials ----------------------
// Synthetic starter list made from the compiled table (kZiruStarterSerials): the 70 real
// card ids, either under build 138's shared serials or under Konami's own serials.
static const int kSquadPositions[23] = {66, 33, 35, 56, 54, 23, 67, 40, 53, 37, 59, 52, 41, 29, 30, 18, 69, 60, 31, 63, 65, 48, 58};
static const int kStarters = (int)(sizeof(kZiruStarterSerials) / sizeof(kZiruStarterSerials[0]));
static long long starterSerial(int pos, bool konami) { return konami ? kZiruStarterSerials[pos].now : kZiruStarterSerials[pos].before; }
static ObjP starterCard(int pos, bool konami) {
  const std::vector<int> none = {0, 0, 0, 0, 0, 0, 0};
  auto c = card(kZiruStarterSerials[pos].id, starterSerial(pos, konami), 9, 0, 0, none, {}, 108, 0, 145);
  c->getJSONArray("parameters")->put(47, 40 + pos);  // every entry differs, also the two copies of one card
  c->put("level_info", Object::parse("{\"level\":" + std::to_string(1 + pos % 30) + ",\"level_limit\":" + std::to_string(1 + pos % 30) + ",\"customize_point\":0}"));
  return c;
}
// The ids the iOS client knows two starter cards by (z27UpgradeIosCardPid).
static long long shownId(long long id) { return id == 40352 ? 89133993205152LL : id == 44379 ? 106788187843931LL : id; }
static void writeStarterAssets(const std::string& root, bool konami) {
  std::filesystem::create_directories(root + "/responses");
  auto list = newArr(), refs = newArr();
  for (int pos = 0; pos < kStarters; ++pos) list->put(starterCard(pos, konami));
  for (int slot = 0; slot < 23; ++slot) {
    int pos = kSquadPositions[slot];
    refs->put(newObj()->put("gameplayer_id", newObj()->put("id", kZiruStarterSerials[pos].id)->put("serial", starterSerial(pos, konami)))->put("uniform_number", slot + 1));
  }
  auto squad = newObj()->put("player_list", refs)->put("coach", newArr()->put(coachRef(100, 1000)))->put("squad_gameplan", newObj());
  asset(root, "CmdLogin.json", newObj()->put("entry_info", newObj()->put("main_squad_id", 9)->put("gameplayer_list", list)
      ->put("coach_list", newArr()->put(coachRef(100, 1000)))
      ->put("squad_list", newArr()->put(newObj()->put("squad_id", 9)->put("squad_data", squad)->put("base_team", newArr()->put(newObj()->put("base_team_id", 108)))))));
  asset(root, "CmdCreateUser.json", obj(R"({"user_code":"offline"})"));
  asset(root, "ZIRU_MATCH_TEAMS.json", obj(R"({"teams":{"108":{"base_team_id":108,"team_name":"Test","team_name_short":"TST","kits":[{"id":1,"shirt_rgb":[0,0,0]},{"id":2,"shirt_rgb":[255,255,255]}]}}})"));
  const std::vector<int> none = {0, 0, 0, 0, 0, 0, 0};
  auto records = newObj();
  for (long long id : {106788187843931LL, 89133993205152LL, 88045755827028LL, 87962272515241LL})
    records->put(std::to_string(id), newObj()->put("gameplayer", card(id, 0, 9, 0, 0, none, {}, 108, 0, 149)));
  asset(root, "ZIRU_PLAYER_RECORDS.json", records);
  asset(root, "generic.json", obj(R"({"result":"NOERR"})"));
  // Konami's captured MyLeague match and result are of the same account: they name
  // starter cards under Konami's serials.
  auto captured = newArr(), stats = newArr();
  for (int pos : {66, 54, 23}) {
    auto c = card(kZiruStarterSerials[pos].id, kZiruStarterSerials[pos].now, 9, 0, 0, none, {}, 108, 0, 145);
    captured->put(newObj()->put("gameplayer_id", c->getJSONObject("gameplayer_id"))->put("parameters", c->getJSONArray("parameters")));
    stats->put(newObj()->put("gameplayer_id", Object::parse(c->getJSONObject("gameplayer_id")->toString()))->put("games", newArr()->put(198))
        ->put("goal", newArr()->put(24))->put("assist", newArr()->put(39)));
  }
  asset(root, "ZIRU_ML_START_GAME.json", newObj()->put("squad_data_home", newObj()->put("player_list", captured)
      ->put("coach", newObj()->put("coach_id", newObj()->put("id", 100)->put("serial", 1000)))->put("squad_gameplan", newObj()))->put("commentary_info", newObj()));
  auto advice = newArr()->put(newObj()->put("advice_info_list", newArr()->put(newObj()->put("gameplayer_id", Object::parse(stats->getJSONObject(0)->getJSONObject("gameplayer_id")->toString())))));
  asset(root, "ZIRU_ML_GAME_RESULT.json", newObj()->put("game_end_result", newObj()->put("match_result", newObj()->put("score_data", newObj()))
      ->put("extra_match_result", newObj()->put("gameplayer_update_info", stats))
      ->put("analyst_get_game_result_info", newObj()->put("advice_info_match_list", advice))));
}
// Every row, careerplan point, squad, meta value (but the backup) and pref of a save.
static std::map<std::string, std::string> saveDump(OfflineServer& s) {
  std::map<std::string, std::string> out;
  for (const char* q : {"SELECT 'player ' || serial, player_id || ' ' || player_json FROM players",
                        "SELECT 'progression ' || serial, progression_json FROM progression",
                        "SELECT 'squad ' || squad_id, squad_json FROM squads",
                        "SELECT 'meta ' || k, v FROM meta WHERE k != 'z27_starter_serials_backup'"}) {
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(s.ziruDb->db, q, -1, &st, nullptr);
    while (sqlite3_step(st) == SQLITE_ROW) out[(const char*)sqlite3_column_text(st, 0)] = (const char*)sqlite3_column_text(st, 1);
    sqlite3_finalize(st);
  }
  auto& prefs = s.sp("almanac_state");
  for (const auto& k : prefs.data->keys()) out["prefs " + k] = prefs.data->optJSONObject(k)->toString();
  return out;
}
static std::string firstDifference(const std::map<std::string, std::string>& a, const std::map<std::string, std::string>& b) {
  for (auto& kv : a) if (!b.count(kv.first) || b.at(kv.first) != kv.second) return kv.first;
  for (auto& kv : b) if (!a.count(kv.first)) return kv.first;
  return "";
}

static void starterSerialTests(const std::string& top) {
  const long long DUP1 = 17592454614461LL, DUP2 = 88035823874881LL, OLD = 359042717000000000LL;
  auto oldSerial = [&](int last) { return OLD + last; };
  // ---- A new save with Konami's serials: every starter card is its own card ----
  {
    std::string root = top + "/serials-fresh";
    writeStarterAssets(root, true);
    Config cfg; cfg.assetsDir = root; cfg.dataDir = root + "/save";
    std::filesystem::create_directories(cfg.dataDir);
    OfflineServer s(cfg);
    call(s, "CMD_CREATE_USER");
    auto login = call(s, "CMD_LOGIN")->getJSONObject("entry_info")->getJSONArray("gameplayer_list");
    auto entry = call(s, "CMD_GET_MYCLUB_ENTRY_INFO")->getJSONObject("entry_info")->getJSONArray("gameplayer_list");
    for (auto list : {login, entry}) {
      std::set<long long> serials;
      check(list->length() == kStarters, ("every starter card is listed, got " + std::to_string(list->length())).c_str());
      for (int pos = 0; pos < kStarters && pos < list->length(); ++pos) {
        auto gid = list->getJSONObject(pos)->getJSONObject("gameplayer_id");
        serials.insert(gid->getLong("serial"));
        check(gid->getLong("serial") == kZiruStarterSerials[pos].now && gid->getLong("id") == shownId(kZiruStarterSerials[pos].id) &&
              list->getJSONObject(pos)->getJSONArray("parameters")->getInt(47) == 40 + pos,
            ("starter card " + std::to_string(pos) + " is sent as itself under its own serial").c_str());
      }
      check((int)serials.size() == kStarters, "70 cards, 70 serials");
    }
    check((int)s.ziruDb->loadAllPlayerRows().size() == kStarters, "a row for every starter card");
    check(s.loadMeta("z27_starter_serials_v1") && !s.loadMeta("z27_starter_serials_backup"), "a new save only gets the done flag");
    // MyLeague: the CPU team plays Konami's captured squad. Those cards are not the user's,
    // and the captured result's per-card stats and advice are not about the user's cards.
    std::set<std::pair<long long, long long>> mine;
    for (int i = 0; i < login->length(); ++i) {
      auto g = login->getJSONObject(i)->getJSONObject("gameplayer_id");
      mine.insert({g->getLong("id"), g->getLong("serial")});
    }
    s.ziruMyLeagueSetActive(1);
    auto cpu = call(s, "CMD_START_GAME", newObj()->put("game_id", 7))->getJSONObject("squad_data_away")->getJSONArray("player_list");
    check(cpu->length() == 3, "MyLeague: the CPU team plays the captured squad");
    for (int i = 0; i < cpu->length(); ++i) {
      auto g = cpu->getJSONObject(i)->getJSONObject("gameplayer_id");
      check(!mine.count({g->getLong("id"), g->getLong("serial")}), "MyLeague: no CPU card has the id and serial of a card the user owns");
    }
    auto result = call(s, "CMD_GET_VSCOM_GAME_RESULT", newObj()->put("game_id", 7)
        ->put("score", newArr()->put(newObj()->put("score_1st", 1))->put(newObj()->put("score_1st", 0))))->getJSONObject("game_end_result");
    check(result->getJSONObject("extra_match_result")->getJSONArray("gameplayer_update_info")->length() == 0 &&
          result->getJSONObject("analyst_get_game_result_info")->getJSONArray("advice_info_match_list")->length() == 0,
        "MyLeague result: no captured stats or advice for the user's cards");
    s.ziruMyLeagueSetActive(0);
  }
  // ---- A save written by build 138, with the shared serials ----
  std::string root = top + "/serials-old";
  writeStarterAssets(root, true);
  const long long PACK = 87962272515241LL;  // a pack card with a starter card's id, at pack serial 31
  auto writeOldSave = [&](const std::string& dir) {
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    Config cfg; cfg.assetsDir = root; cfg.dataDir = dir;
    OfflineServer s(cfg);
    auto& prefs = s.sp("almanac_state");
    auto edit = prefs.edit();
    std::set<long long> done;
    int k = 0;
    for (int pos = 0; pos < kStarters; ++pos) {
      long long serial = kZiruStarterSerials[pos].before;
      if (!done.insert(serial).second || serial == oldSerial(260)) continue;  // the first card listed under a serial is the row card
      auto row = starterCard(pos, false);
      row->getJSONObject("gameplayer_id")->put("id", shownId(kZiruStarterSerials[pos].id));
      row->put("additional_skill_id", newArr()->put(44)->put(k % 2 ? 45 : 46));
      row->put("initial_booster_id_list", newArr()->put(100 + k)->put(0)->put(0));
      row->getJSONArray("parameters")->put(142, 100 + k)->put(kZiruSkillParameters[44], 1);
      row->put("additional_position", newArr()->put("CF"));
      s.ziruDb->savePlayer(serial, shownId(kZiruStarterSerials[pos].id), row->toString());
      edit.putString("ziru_skills_" + std::to_string(serial), "44," + std::to_string(k % 2 ? 45 : 46));
      k++;
    }
    s.ziruDb->saveProgression(oldSerial(0), R"({"SHOOT":6,"_spent_total":6})");
    s.ziruDb->saveProgression(oldSerial(600), R"({"PASS":4,"_spent_total":4})");
    // 260 was sold: build 138 removed its row, the learned skills stayed in prefs.
    s.ziruDb->saveMeta("sold_player_" + std::to_string(oldSerial(260)), "1");
    edit.putString("ziru_skills_" + std::to_string(oldSerial(260)), "33");
    auto pack = card(PACK, 31, 9, 0, 0, {0, 0, 0, 0, 0, 0, 0}, {}, 108, 0, 145);
    s.ziruDb->savePlayer(31, PACK, pack->toString());
    edit.putString("recruited_players", newArr()->put(newObj()->put("gameplayer", pack))->toString()).putLong("ziru_next_serial", 31)
        .putString("ziru_skills_31", "12");
    edit.commit();
    // Squads as build 138 saved them (ids already upgraded): the default squad, and one with
    // both copies of the two cards listed twice under one serial, upgraded ids, a pack card,
    // a rental and a card no starter list has.
    auto squadOf = [](std::vector<std::pair<long long, long long>> refs) {
      auto list = newArr();
      for (auto& r : refs) list->put(newObj()->put("gameplayer_id", newObj()->put("id", r.first)->put("serial", r.second))->put("uniform_number", list->length() + 1));
      return newObj()->put("player_list", list)->put("coach", newArr()->put(coachRef(100, 1000)))->put("squad_gameplan", newObj());
    };
    std::vector<std::pair<long long, long long>> main;
    for (int pos : kSquadPositions) main.push_back({shownId(kZiruStarterSerials[pos].id), kZiruStarterSerials[pos].before});
    s.ziruDb->saveSquad(9, squadOf(main)->toString());
    s.ziruDb->saveSquad(1, newObj()->put("squad_data", squadOf({{DUP1, oldSerial(200)}, {DUP2, oldSerial(600)}, {DUP2, oldSerial(600)}, {DUP1, oldSerial(200)},
        {106788187843931LL, oldSerial(60)}, {89133993205152LL, oldSerial(100)}, {17592454614509LL, oldSerial(60)}, {PACK, 31},
        {88032334187994LL, 0}, {990000000003LL, oldSerial(640)}}))->toString());
    s.ziruDb->saveMeta("ziru_myleague_special_squad", newObj()->put("squad_data", squadOf({{88045755827028LL, oldSerial(640)}, {DUP2, oldSerial(600)},
        {DUP2, oldSerial(600)}, {88032334187994LL, 0}}))->toString());
    s.ziruDb->saveMeta("z27_active_match", newObj()->put("game_id", 5)->put("home_squad", squadOf(main))->toString());
  };
  auto newSerialOf = [&](int pos) { return kZiruStarterSerials[pos].now; };
  auto refsOf = [](const ObjP& squad) {
    ObjP data = squad->optJSONObject("squad_data") ? squad->getJSONObject("squad_data") : squad;
    std::vector<std::pair<long long, long long>> out;
    auto list = data->getJSONArray("player_list");
    for (int i = 0; i < list->length(); ++i) {
      auto g = list->getJSONObject(i)->getJSONObject("gameplayer_id");
      out.push_back({g->getLong("id"), g->getLong("serial")});
    }
    return out;
  };
  {
    std::string dir = root + "/save";
    writeOldSave(dir);
    Config cfg; cfg.assetsDir = root; cfg.dataDir = dir;
    std::map<long long, std::string> oldRows, oldSkills;
    std::string pack31, recruited;
    {
      OfflineServer s(cfg);
      s.z27MigrateSavedCards();  // the repair of older saves runs first; compare with what it leaves
      for (auto& [serial, text] : s.ziruDb->loadAllPlayerRows()) {
        oldRows[serial] = text;
        oldSkills[serial] = s.sp("almanac_state").getString("ziru_skills_" + std::to_string(serial), "");
      }
      pack31 = *s.loadPlayer(31);
      recruited = s.sp("almanac_state").getString("recruited_players", "");
    }
    OfflineServer s(cfg);
    call(s, "CMD_GET_MANNER_WARN");  // any request: the save is moved before it is handled
    check(s.loadMeta("z27_starter_serials_v1") && s.loadMeta("z27_starter_serials_backup") && !s.loadMeta("z27_starter_serials_prefs"),
        "old save: moved once, with a backup, the prefs written");
    auto& prefs = s.sp("almanac_state");
    // The row card of each old serial is now under its own serial, exactly as it was saved, and
    // its learned skills and careerplan points went with it.
    std::set<long long> done;
    for (int pos = 0; pos < kStarters; ++pos) {
      long long before = kZiruStarterSerials[pos].before;
      if (!done.insert(before).second || before == oldSerial(260)) continue;
      auto expected = Object::parse(oldRows.at(before));
      expected->getJSONObject("gameplayer_id")->put("serial", (long long)newSerialOf(pos));
      JStr moved = s.loadPlayer(newSerialOf(pos));
      check(moved && *moved == expected->toString(), ("old save: the row card of " + std::to_string(before) + " is under " + std::to_string(newSerialOf(pos))).c_str());
      check(!oldSkills.at(before).empty() && prefs.getString("ziru_skills_" + std::to_string(newSerialOf(pos)), "") == oldSkills.at(before),
            "old save: the learned skills moved with the card");
    }
    for (long long last : {0, 60, 100, 200, 260, 300, 400, 450, 500, 600, 640}) {
      bool newHome = last == 60 || last == 600;  // also Konami's serial of another starter card, which starts fresh
      check(!s.loadPlayer(oldSerial(last)) && !s.loadProgression(oldSerial(last)) && !prefs.hasString("ziru_skills_" + std::to_string(oldSerial(last))),
          ("old save: nothing is left under old serial " + std::to_string(last) + (newHome ? " (now another card's serial)" : "")).c_str());
    }
    check(*s.loadProgression(newSerialOf(0)) == R"({"SHOOT":6,"_spent_total":6})" && *s.loadProgression(newSerialOf(47)) == R"({"PASS":4,"_spent_total":4})",
        "old save: careerplan points moved with the card");
    check(!s.isZiruPlayerSold(oldSerial(260)) && s.isZiruPlayerSold(newSerialOf(26)) && s.isZiruPlayerSold(newSerialOf(27)) &&
          !s.loadPlayer(newSerialOf(26)) && !prefs.hasString("ziru_skills_" + std::to_string(newSerialOf(26))),
        "old save: the cards listed under a sold serial stay hidden; the sold serial's leftovers are dropped");
    check(*s.loadPlayer(31) == pack31 && prefs.getString("recruited_players", "") == recruited && prefs.getLong("ziru_next_serial", 0) == 31 &&
          prefs.getString("ziru_skills_31", "") == "12", "old save: pack cards are not touched, also one with a starter card's id");
    // Squads: every reference to an old serial points at the new one.
    std::vector<std::pair<long long, long long>> mainNew;
    for (int pos : kSquadPositions) mainNew.push_back({shownId(kZiruStarterSerials[pos].id), newSerialOf(pos)});
    check(refsOf(Object::parse(*s.loadSquad(9))) == mainNew, "old save: the main squad holds the same cards under their new serials");
    auto custom = refsOf(Object::parse(*s.loadSquad(1)));
    std::vector<std::pair<long long, long long>> customNew = {{DUP1, newSerialOf(23)}, {DUP2, newSerialOf(54)}, {DUP2, newSerialOf(50)}, {DUP1, newSerialOf(22)},
        {106788187843931LL, newSerialOf(4)}, {89133993205152LL, newSerialOf(10)}, {17592454614509LL, newSerialOf(5)}, {PACK, 31},
        {88032334187994LL, 0}, {990000000003LL, oldSerial(640)}};
    check(custom == customNew, "old save: a card listed twice gets Konami's squad copy first, the other copy second; upgraded ids, pack, rental and unknown references as expected");
    auto special = refsOf(Object::parse(*s.loadMeta("ziru_myleague_special_squad")));
    check(special == std::vector<std::pair<long long, long long>>({{88045755827028LL, newSerialOf(66)}, {DUP2, newSerialOf(54)}, {DUP2, newSerialOf(50)}, {88032334187994LL, 0}}),
        "old save: the MyLeague special squad points at the new serials");
    check(!s.z27Meta("z27_active_match")->has("home_squad"), "old save: the cached match squad is rebuilt at the next match start");
    // A second start changes nothing.
    auto moved = saveDump(s);
    {
      OfflineServer again(cfg);
      call(again, "CMD_GET_MANNER_WARN");
      std::string diff = firstDifference(moved, saveDump(again));
      check(diff.empty(), ("old save: a second start changes nothing, " + diff).c_str());
    }
    // The login lists every starter card under its own serial: the moved cards with everything they had.
    auto login = call(s, "CMD_LOGIN")->getJSONObject("entry_info")->getJSONArray("gameplayer_list");
    check(login->length() == kStarters - 2 + 1, ("old save: the login lists 68 starter cards and the pack card, got " + std::to_string(login->length())).c_str());
    auto movedCard = bySerial(login, newSerialOf(4));
    check(movedCard->getJSONObject("gameplayer_id")->getLong("id") == 106788187843931LL && movedCard->getJSONArray("additional_skill_id")->getInt(0) == 44 &&
          movedCard->getJSONArray("initial_booster_id_list")->getInt(0) == 101 && movedCard->getJSONArray("additional_position")->getString(0) == "CF",
        "old save: a moved card keeps its learned skills, booster and position");
    auto fresh = bySerial(login, newSerialOf(5));
    check(fresh->getJSONObject("gameplayer_id")->getLong("id") == 17592454614509LL && fresh->getJSONArray("additional_skill_id")->length() == 0 &&
          fresh->getJSONArray("initial_booster_id_list")->getInt(0) == 0 && !fresh->has("additional_position"),
        "old save: the card Konami lists under an old serial starts with its starter values");
    check(bySerial(login, newSerialOf(22))->getJSONArray("parameters")->getInt(47) == 62 && bySerial(login, newSerialOf(23))->getJSONArray("parameters")->getInt(47) == 63,
        "old save: both copies of a card listed twice are listed, each as itself");
  }
  // ---- Stopped between the database and the prefs: the prefs part is applied at the next start ----
  {
    std::string dir = root + "/save-replay";
    writeOldSave(dir);
    Config cfg; cfg.assetsDir = root; cfg.dataDir = dir;
    std::string before;
    {
      OfflineServer s(cfg);
      before = s.sp("almanac_state").getString("ziru_skills_" + std::to_string(oldSerial(0)), "");
      call(s, "CMD_GET_MANNER_WARN");
      s.ziruDb->saveMeta("z27_starter_serials_prefs", newObj()->put("ziru_skills_" + std::to_string(newSerialOf(0)), "44,46")
          ->put("ziru_skills_" + std::to_string(oldSerial(0)), Value::null())->toString());
      s.sp("almanac_state").edit().putString("ziru_skills_" + std::to_string(oldSerial(0)), before)
          .remove("ziru_skills_" + std::to_string(newSerialOf(0))).commit();
    }
    OfflineServer s(cfg);
    call(s, "CMD_GET_MANNER_WARN");
    check(!s.loadMeta("z27_starter_serials_prefs") && s.sp("almanac_state").getString("ziru_skills_" + std::to_string(newSerialOf(0)), "") == "44,46" &&
          !s.sp("almanac_state").hasString("ziru_skills_" + std::to_string(oldSerial(0))), "a prefs plan left behind is applied at the next start");
  }
  // ---- A failure inside the move leaves the save as it was; the next request tries again ----
  {
    std::string dir = root + "/save-fail";
    writeOldSave(dir);
    Config cfg; cfg.assetsDir = root; cfg.dataDir = dir;
    OfflineServer s(cfg);
    s.ziruDb->saveMeta("z27_card_identity_v2", "1");
    auto before = saveDump(s);
    check(sqlite3_exec(s.ziruDb->db, "CREATE TRIGGER fail_move BEFORE INSERT ON meta WHEN NEW.k='z27_starter_serials_v1' BEGIN SELECT RAISE(ABORT, 'test'); END",
                       nullptr, nullptr, nullptr) == SQLITE_OK, "install the failing write");
    call(s, "CMD_GET_MANNER_WARN");
    std::string diff = firstDifference(before, saveDump(s));
    check(diff.empty() && !s.loadMeta("z27_starter_serials_backup"), ("a failed move leaves the save as it was, " + diff).c_str());
    sqlite3_exec(s.ziruDb->db, "DROP TRIGGER fail_move", nullptr, nullptr, nullptr);
    call(s, "CMD_GET_MANNER_WARN");
    check(s.loadMeta("z27_starter_serials_v1") && s.loadPlayer(newSerialOf(0)), "the next request moves the save");
  }
  // ---- Old starter data: the save waits, unchanged ----
  {
    std::string oldRoot = top + "/serials-old-data";
    writeStarterAssets(oldRoot, false);
    std::string dir = oldRoot + "/save";
    writeOldSave(dir);
    Config cfg; cfg.assetsDir = oldRoot; cfg.dataDir = dir;
    OfflineServer s(cfg);
    s.ziruDb->saveMeta("z27_card_identity_v2", "1");
    auto before = saveDump(s);
    call(s, "CMD_GET_MANNER_WARN");
    std::string diff = firstDifference(before, saveDump(s));
    check(diff.empty(), ("old starter data: the save is not moved, " + diff).c_str());
    auto login = call(s, "CMD_LOGIN")->getJSONObject("entry_info")->getJSONArray("gameplayer_list");
    check(!s.loadMeta("z27_starter_serials_v1") && bySerial(login, oldSerial(60))->getJSONObject("gameplayer_id")->getLong("id") == 106788187843931LL,
        "old starter data: the login works on the old serials as before");
  }
}

// ---- Position training as the 11.1.1 client does it (PESmobile, disassembled) -------------
// Written from the client, not from the server, so the tests check the server against it.
// Position numbers (CommandCamPosition literals, 0x10399da00): GK 0, CB 1, LSB 2, RSB 3, DMF 4,
// CMF 5, LMF 6, RMF 7, OMF 8, LWG 9, RWG 10, ST 11, CF 12; 13 is an empty learned-position slot.
static const char* const kPositionNames[13] = {"GK", "CB", "LSB", "RSB", "DMF", "CMF", "LMF", "RMF", "OMF", "LWG", "RWG", "ST", "CF"};
// The parameters[] index of each position's level (tmpdb::Player::GetDataLabel(Position), 0x10578ba08).
static const int kPositionLabels[13] = {13, 10, 11, 12, 6, 7, 8, 9, 5, 3, 4, 2, 1};
// The positions next to each registered position, which a card may learn even at level 0
// (built by __GLOBAL__sub_I_OnlineModeUtilityMyClubTraining.cpp, 0x1048a5d44, at 0x10764b698).
static const std::vector<int> kPositionsNextTo[13] = {{}, {2, 3, 4}, {1, 6}, {1, 7}, {1, 5, 6, 7}, {4, 6, 7, 8},
    {2, 4, 5, 8, 9}, {3, 4, 5, 8, 10}, {6, 7, 5, 11}, {6, 11, 12}, {7, 11, 12}, {9, 10, 8, 12}, {9, 10, 11}};
struct ClientCard { int registered = 0; int level[13] = {}; int slot[2] = {13, 13}; };
static int positionNumber(const std::string& name) {
  for (int p = 0; p < 13; ++p) if (name == kPositionNames[p]) return p;
  return -1;
}
// The card as the client holds it after a login (ModeUtility::SetCommandGamePlayerToMyClubGamePlayer,
// 0x104703a64): the parameters as sent, and the additional_position names as positions.
static ClientCard clientCard(const ObjP& card) {
  ClientCard c;
  auto values = card->getJSONArray("parameters");
  c.registered = values->getInt(0);
  for (int p = 0; p < 13; ++p) c.level[p] = values->getInt(kPositionLabels[p]);
  auto learned = card->optJSONArray("additional_position");
  for (int i = 0; learned && i < learned->length() && i < 2; ++i) c.slot[i] = positionNumber(learned->getString(i));
  return c;
}
// Whether the Position Training (Select) screen lets the player pick `p` for `slot`:
// MyClubMyTeamPlayerTrainingPositionSelect::AvailablePositionTraining (0x10468321c) with
// UtilityMyClubTraining::GetTrainablePositionList (0x1048a4ba4) and AddToPositionList (0x1048a4cfc).
static bool clientOffers(const ClientCard& c, int slot, int p) {
  if (c.level[p] > 1 || c.slot[slot] == p) return false;
  const auto& next = kPositionsNextTo[c.registered];
  bool listed = (c.level[p] != 0 && p != c.registered) || std::count(next.begin(), next.end(), p) > 0;
  return listed && (c.level[p] != 2 || c.slot[0] == p || c.slot[1] == p);
}
// What the client does to the card after is_success "YES" (CommandObjectCmdUseMyclubItem::QuitCommand,
// 0x104774eac..0x104774f24): the position the slot held loses a level, the new one gains a level.
static void clientLearns(ClientCard& c, int slot, int p) {
  if (c.slot[slot] != 13) c.level[c.slot[slot]] -= 1;
  c.level[p] += 1;
  c.slot[slot] = p;
}
static std::string describe(const ClientCard& c) {
  std::string out = "registered " + std::to_string(c.registered) + " levels";
  for (int p = 0; p < 13; ++p) out += " " + std::string(kPositionNames[p]) + "=" + std::to_string(c.level[p]);
  return out + " slots " + std::to_string(c.slot[0]) + "," + std::to_string(c.slot[1]);
}
static bool sameCard(const ClientCard& a, const ClientCard& b) { return describe(a) == describe(b); }

// ---- Skill and Position Training (Select), as the 11.1.1 client sends them ----------------
// CmdUseMyclubItemOption::PackMsgPack (0x103b120a4) sends item_list [{item {item_type, grade,
// effect}, present_num}], auto_item_list, is_auto_use_item, gameplayer_id_list, training_index,
// ability_id and position_id; CommandObjectCmdUseMyclubItem::InitCommand (0x1047746e0) fills them:
//   SKILL_TRAINING_SELECT:    effect = ability_id = the picked skill, client numbers; position_id ""
//   POSITION_TRAINING_SELECT: effect = the picked Position (GK 0 ... CF 12), position_id = its name
//                             ("CF", "ST", "LWG", "OMF", "LSB", ...); ability_id is not filled in
static void selectTrainingTests(const std::string& top) {
  const std::string root = top + "/select-training";
  std::filesystem::create_directories(root + "/responses");
  const std::vector<int> none = {0, 0, 0, 0, 0, 0, 0};
  const long long P = 7 + (1LL << 40);   // LWF: plays LWG (2), ST only a little (1); own skill Double Touch (1)
  const long long Q = 8 + (1LL << 40);   // GK
  auto lwf = card(P, 0, 9, 9, 0, none, {1}, 108, 0, 149);
  lwf->getJSONArray("parameters")->put(3, 2)->put(2, 1);
  auto gk = card(Q, 0, 0, 0, 17, none, {}, 108, 0, 149);
  gk->getJSONArray("parameters")->put(13, 2);
  asset(root, "ZIRU_PLAYER_RECORDS.json", newObj()->put(std::to_string(P), newObj()->put("gameplayer", lwf))
      ->put(std::to_string(Q), newObj()->put("gameplayer", gk)));
  auto starterP = card(P, 701, 9, 9, 0, none, {1}, 108, 0, 145);
  starterP->getJSONArray("parameters")->put(3, 2)->put(2, 1);
  auto starterQ = card(Q, 702, 0, 0, 17, none, {}, 108, 0, 145);
  starterQ->getJSONArray("parameters")->put(13, 2);
  auto refs = newArr()->put(newObj()->put("gameplayer_id", newObj()->put("id", P)->put("serial", 701)))
      ->put(newObj()->put("gameplayer_id", newObj()->put("id", Q)->put("serial", 702)));
  auto squad = newObj()->put("player_list", refs)->put("coach", newArr()->put(coachRef(100, 1000)))->put("squad_gameplan", newObj());
  asset(root, "CmdLogin.json", newObj()->put("entry_info", newObj()->put("main_squad_id", 9)->put("gameplayer_list", newArr()->put(starterP)->put(starterQ))
      ->put("coach_list", newArr()->put(coachRef(100, 1000)))
      ->put("squad_list", newArr()->put(newObj()->put("squad_id", 9)->put("squad_data", squad)->put("base_team", newArr()->put(newObj()->put("base_team_id", 108)))))));
  asset(root, "CmdCreateUser.json", obj(R"({"user_code":"offline"})"));
  asset(root, "ZIRU_MATCH_TEAMS.json", obj(R"({"teams":{"108":{"base_team_id":108,"team_name":"Test","team_name_short":"TST","kits":[{"id":1,"shirt_rgb":[0,0,0]},{"id":2,"shirt_rgb":[255,255,255]}]}}})"));
  // Konami's answer template carries a successful skill result of its own.
  asset(root, "CMD_USE_MYCLUB_ITEM.json", obj(R"({"result":"NOERR","skill_training_result":{"additional_skill_id":22,"is_success":"YES"},
      "item_list":[{"item":{"item_type":"POSITION_TRAINING_SELECT","grade":0,"effect":0},"inventory_num":1}]})"));
  asset(root, "generic.json", obj(R"({"result":"NOERR"})"));
  Config cfg; cfg.assetsDir = root; cfg.dataDir = root + "/save"; cfg.iosPlayerLayout = true;
  std::filesystem::create_directories(cfg.dataDir);
  OfflineServer s(cfg);
  call(s, "CMD_LOGIN");
  int lastAbility = 0;  // sent as ability_id with a position pick: whatever it holds, it is not the pick
  auto use = [&](const char* type, long long id, long long serial, int index, int effect, int ability, ObjP extra) {
    auto req = newObj()->put("item_list", newArr()->put(newObj()->put("item", newObj()->put("item_type", type)->put("grade", 0)->put("effect", effect))
                                                        ->put("present_num", 100)))
        ->put("auto_item_list", newArr())->put("is_auto_use_item", "NO")
        ->put("gameplayer_id_list", newArr()->put(newObj()->put("id", id)->put("serial", serial)))
        ->put("training_index", index)->put("ability_id", ability);
    for (const auto& k : extra->keys()) req->put(k, extra->get(k));
    return call(s, "CMD_USE_MYCLUB_ITEM", req);
  };
  auto skillSelect = [&](int wire, int index) {
    lastAbility = wire;
    return use("SKILL_TRAINING_SELECT", P, 701, index, wire, wire, newObj()->put("position_id", ""));
  };
  auto positionSelect = [&](long long id, long long serial, int index, int position, const char* name) {
    return use("POSITION_TRAINING_SELECT", id, serial, index, position, lastAbility, newObj()->put("position_id", name));
  };
  auto saved = [&](long long serial) { return Object::parse(*s.loadPlayer(serial)); };
  auto itemsKept = [](const ObjP& r) {
    auto items = r->getJSONArray("item_list");
    for (int i = 0; i < items->length(); ++i) if (items->getJSONObject(i)->getInt("inventory_num") != 999999) return false;
    return items->length() > 0;
  };

  // Skill Training (Select): every skill the client offers (UtilityMyClubTraining::GetCategorySkillIdList,
  // 0x1048a5600, all six lists), each picked into the same slot, is the skill taught and answered.
  const std::vector<int> offered = {28, 32, 33, 34, 35, 37, 38, 39, 44, 0, 2, 3, 4, 5, 6, 7, 10, 56, 57, 58, 59, 60, 61, 65,
                                    17, 19, 20, 21, 22, 23, 27, 29, 49, 45, 46, 48, 50, 72, 15, 47, 55, 69, 71};
  for (int wire : offered) {
    int internal = iosSkillId(wire, false);
    check(isZiruTrainableSkill(internal), "every skill the client offers is a normal skill");
    for (int round = 0; round < 3; ++round) {
      srand(round * 7919 + wire);
      auto r = skillSelect(wire, 0)->getJSONObject("skill_training_result");
      check(r->getString("is_success") == "YES" && r->getInt("additional_skill_id") == wire,
          ("Skill Select: the picked skill is answered, in the client's numbers: " + std::to_string(wire)).c_str());
      check(s.sp("almanac_state").getString("ziru_skills_701", "") == std::to_string(internal) && skillBit(saved(701), internal) == 1,
          ("Skill Select: the picked skill is the one learned: " + std::to_string(internal)).c_str());
    }
  }
  // The card's own skill (Double Touch, client 1) is refused, not swapped for another one.
  {
    auto before = saveDump(s);
    auto r = skillSelect(1, 1);
    check(r->getJSONObject("skill_training_result")->getString("is_success") == "NO" && !r->getJSONObject("skill_training_result")->has("additional_skill_id") &&
          firstDifference(before, saveDump(s)).empty() && itemsKept(r), "Skill Select: a skill the card has is refused; nothing changes, no item used up");
  }

  // Position Training (Select): the picked position, every time, in either slot. P is an LWG
  // (level 2) who plays ST a little (1); next to LWG are LMF, ST and CF.
  auto shown = [&](long long serial) { return clientCard(bySerial(call(s, "CMD_LOGIN")->getJSONObject("entry_info")->getJSONArray("gameplayer_list"), serial)); };
  struct Pick { int index, position; const char* name; };
  for (Pick pick : {Pick{0, 12, "CF"}, Pick{1, 6, "LMF"}, Pick{0, 11, "ST"}}) {
    ClientCard expected = shown(701);
    check(clientOffers(expected, pick.index, pick.position), (std::string("the client offers ") + pick.name).c_str());
    clientLearns(expected, pick.index, pick.position);
    auto r = positionSelect(P, 701, pick.index, pick.position, pick.name)->getJSONObject("position_training_result");
    check(r->getString("is_success") == "YES" && r->getString("additional_position") == pick.name,
        (std::string("Position Select: the picked position is answered: ") + pick.name).c_str());
    check(sameCard(shown(701), expected), (std::string("Position Select: the card learned ") + pick.name + " as the client shows it: " + describe(shown(701))).c_str());
  }
  {
    // Slot 0 now holds ST (it replaced CF), slot 1 LMF: ST 1 + 1, LMF 0 + 1, CF back to 0, LWG as it was.
    auto row = saved(701);
    auto values = row->getJSONArray("parameters");
    check(row->getJSONArray("additional_position")->toString() == "[\"ST\",\"LMF\"]" && values->getInt(1) == 0 && values->getInt(2) == 2 &&
          values->getInt(8) == 1 && values->getInt(3) == 2, "Position Select: a learned position is one level up; a replaced one goes back to the card's own level");
  }
  // Refused: what the client does not offer. Its own position, the position this slot holds,
  // a position neither played nor next to LWG, GK, and any card that is a GK.
  struct Refusal { const char* what; long long id, serial; int index, position; const char* name; };
  for (Refusal no : {Refusal{"its own position", P, 701, 0, 9, "LWG"}, Refusal{"the position this slot holds", P, 701, 1, 6, "LMF"},
                     Refusal{"a position it does not play and that is not next to its own", P, 701, 0, 4, "DMF"},
                     Refusal{"GK", P, 701, 1, 0, "GK"}, Refusal{"a GK card", Q, 702, 0, 12, "CF"}}) {
    auto before = saveDump(s);
    auto r = positionSelect(no.id, no.serial, no.index, no.position, no.name);
    auto result = r->getJSONObject("position_training_result");
    check(result->getString("is_success") == "NO" && result->getString("additional_position").empty() && firstDifference(before, saveDump(s)).empty() && itemsKept(r),
        (std::string("Position Select: ") + no.what + " is refused, nothing changes and no item is used up").c_str());
    check(!r->has("skill_training_result") && !r->has("ability_training_result"), (std::string("Position Select: ") + no.what + ": only the position result is sent").c_str());
  }
  // Without position_id the item's effect is the picked Position; a number in position_id too.
  {
    auto r = use("POSITION_TRAINING_SELECT", P, 701, 0, 12, 0, newObj())->getJSONObject("position_training_result");
    check(r->getString("is_success") == "YES" && r->getString("additional_position") == "CF", "Position Select: the item's effect names the position");
    r = use("POSITION_TRAINING_SELECT", P, 701, 1, 0, 0, newObj()->put("position_id", 11))->getJSONObject("position_training_result");
    check(r->getString("is_success") == "YES" && r->getString("additional_position") == "ST", "Position Select: a Position number as position_id");
  }
  // Position Training (random): always a position the client would offer for that slot.
  for (int k = 0; k < 20; ++k) {
    ClientCard before = shown(701);
    auto r = use("POSITION_TRAINING", P, 701, k % 2, 0, 0, newObj()->put("position_id", ""))->getJSONObject("position_training_result");
    int got = positionNumber(r->getString("additional_position"));
    check(r->getString("is_success") == "YES" && got >= 0 && clientOffers(before, k % 2, got),
        ("Position Training: a position the client offers, got " + r->getString("additional_position")).c_str());
  }
}

// ---- Position training keeps the card as the client shows it ----------------------------
// After a "YES" the client changes the card itself: one level up for the learned position, one
// down for the position the slot held (clientLearns). Konami's server does the same: a learned
// position is the card's own level plus one. Konami's own login data has three starter cards with
// a learned position: OMF at own level 1 is 2, LWG and RMF at own level 0 are 1.
// Also: each answer carries only the result of the item used, as the client reads only that one
// (QuitCommand 0x104774bc8), and a booster answer carries the card's sell value, which the client
// puts on the card (gameplayer_sell_gp, QuitCommand 0x104774db4).
static void positionTrainingTests(const std::string& top) {
  const std::string root = top + "/position-training";
  std::filesystem::create_directories(root + "/responses");
  const std::vector<int> none = {0, 0, 0, 0, 0, 0, 0};
  auto withLevels = [](const ObjP& c, std::vector<std::pair<int, int>> levels) {
    auto values = c->getJSONArray("parameters");
    for (auto [position, level] : levels) values->put(kPositionLabels[position], level);
    return c;
  };
  // R: an ST with a record: CF 2, ST 2, LWG 1, OMF 1.
  // S: a CMF with only a starter entry: CMF 2, DMF 1, OMF 1, RMF 1.
  // K: an ST with only a starter entry, listed as Konami lists a card that learned LWG (own level 0).
  // L: an LWG with only a starter entry (LWG 2, ST 1), saved by fix2 after it learned ST and then LMF
  //    in slot 0: LMF saved at 2 and ST at 0.
  // G: a goalkeeper.
  const long long R = 21 + (1LL << 40), S = 22 + (1LL << 40), K = 23 + (1LL << 40), L = 24 + (1LL << 40), G = 25 + (1LL << 40);
  const std::vector<std::pair<int, int>> rLevels = {{11, 2}, {12, 2}, {9, 1}, {8, 1}}, sLevels = {{5, 2}, {4, 1}, {8, 1}, {7, 1}};
  asset(root, "ZIRU_PLAYER_RECORDS.json", newObj()->put(std::to_string(R), newObj()->put("gameplayer", withLevels(card(R, 0, 11, 0, 0, none, {}, 108, 0, 149), rLevels))));
  auto starterR = withLevels(card(R, 721, 11, 0, 0, none, {}, 108, 0, 145), rLevels)->put("sell_gp", 230000);
  auto starterS = withLevels(card(S, 722, 5, 0, 0, none, {}, 108, 0, 145), sLevels)->put("sell_gp", 150000);
  auto starterK = withLevels(card(K, 723, 11, 0, 0, none, {}, 108, 0, 145), {{11, 2}, {12, 2}, {9, 1}})->put("additional_position", newArr()->put("LWG"));
  starterK->put("original_parameters", Array::parse(starterK->getJSONArray("parameters")->toString()));
  auto starterL = withLevels(card(L, 724, 9, 0, 0, none, {}, 108, 0, 145), {{9, 2}, {11, 1}});
  auto starterG = withLevels(card(G, 725, 0, 0, 0, none, {}, 108, 0, 145), {{0, 2}});
  auto starters = newArr(), refs = newArr();
  for (auto c : {starterR, starterS, starterK, starterL, starterG}) {
    starters->put(c);
    refs->put(newObj()->put("gameplayer_id", Object::parse(c->getJSONObject("gameplayer_id")->toString())));
  }
  auto squad = newObj()->put("player_list", refs)->put("coach", newArr()->put(coachRef(100, 1000)))->put("squad_gameplan", newObj());
  asset(root, "CmdLogin.json", newObj()->put("entry_info", newObj()->put("main_squad_id", 9)->put("gameplayer_list", starters)
      ->put("coach_list", newArr()->put(coachRef(100, 1000)))
      ->put("squad_list", newArr()->put(newObj()->put("squad_id", 9)->put("squad_data", squad)->put("base_team", newArr()->put(newObj()->put("base_team_id", 108)))))));
  asset(root, "CmdCreateUser.json", obj(R"({"user_code":"offline"})"));
  asset(root, "ZIRU_MATCH_TEAMS.json", obj(R"({"teams":{"108":{"base_team_id":108,"team_name":"Test","team_name_short":"TST","kits":[{"id":1,"shirt_rgb":[0,0,0]},{"id":2,"shirt_rgb":[255,255,255]}]}}})"));
  asset(root, "ZIRU_RANDOM_BOOSTERS.json", obj(R"({"ability_ids":[12,20,30]})"));
  // Konami's answer template is a skill training answer: it carries a successful skill result.
  asset(root, "CMD_USE_MYCLUB_ITEM.json", obj(R"({"result":"NOERR","skill_training_result":{"additional_skill_id":22,"is_success":"YES"},
      "item_list":[{"item":{"item_type":"SKILL_TRAINING","grade":0,"effect":0},"inventory_num":1}],"achievement_notification":[]})"));
  asset(root, "generic.json", obj(R"({"result":"NOERR"})"));
  Config cfg; cfg.assetsDir = root; cfg.dataDir = root + "/save"; cfg.iosPlayerLayout = true;
  std::filesystem::create_directories(cfg.dataDir);
  {
    // L as fix2 saved it.
    OfflineServer first(cfg);
    auto row = withLevels(Object::parse(starterL->toString()), {{6, 2}, {11, 0}})->put("additional_position", newArr()->put("LMF"));
    first.ziruDb->savePlayer(724, L, row->toString());
  }
  OfflineServer s(cfg);
  auto login = [&]() { return call(s, "CMD_LOGIN")->getJSONObject("entry_info")->getJSONArray("gameplayer_list"); };
  auto shown = [&](long long serial) { return clientCard(bySerial(login(), serial)); };
  auto use = [&](const char* type, long long id, long long serial, int index, int effect, ObjP extra) {
    auto req = newObj()->put("item_list", newArr()->put(newObj()->put("item", newObj()->put("item_type", type)->put("grade", 0)->put("effect", effect))
                                                        ->put("present_num", 100)))
        ->put("auto_item_list", newArr())->put("is_auto_use_item", "NO")
        ->put("gameplayer_id_list", newArr()->put(newObj()->put("id", id)->put("serial", serial)))
        ->put("training_index", index)->put("ability_id", 0)->put("position_id", "");
    for (const auto& k : extra->keys()) req->put(k, extra->get(k));
    return call(s, "CMD_USE_MYCLUB_ITEM", req);
  };
  auto pick = [&](long long id, long long serial, int slot, int position) {
    return use("POSITION_TRAINING_SELECT", id, serial, slot, position, newObj()->put("position_id", kPositionNames[position]));
  };
  auto onlyResult = [](const ObjP& r, const char* key) {
    for (const char* k : {"skill_training_result", "ability_training_result", "position_training_result"})
      if ((std::string(k) == key) != r->has(k)) return false;
    return true;
  };

  // The login shows K as Konami lists it, and L as Konami would have it: LMF at 0 + 1, ST at its own 1.
  {
    ClientCard k = shown(723), l = shown(724);
    check(k.level[9] == 1 && k.slot[0] == 9, ("login: a card with Konami's own learned position is listed as Konami lists it, " + describe(k)).c_str());
    check(l.level[6] == 1 && l.level[11] == 1 && l.level[9] == 2 && l.slot[0] == 6,
        ("login: a card fix2 saved with learned positions at level 2 is shown with its own level plus one, " + describe(l)).c_str());
    auto detail = call(s, "CMD_GET_MYCLUB_PLAYER_DATA", newObj()->put("gameplayer_id", newObj()->put("id", L)->put("serial", 724)));
    ClientCard d = clientCard(detail);  // the detail answer has no additional_position
    d.slot[0] = l.slot[0], d.slot[1] = l.slot[1];
    check(sameCard(d, l), ("player detail: the same levels as the login, " + describe(clientCard(detail))).c_str());
  }
  // A learned position at own level 0 is level 1, not 2 (Konami: LWG and RMF at 0 are 1).
  {
    auto r = pick(R, 721, 0, 10);
    check(r->getJSONObject("position_training_result")->getString("is_success") == "YES" && shown(721).level[10] == 1,
        "Position Select: RWG at the card's own level 0 is learned at level 1");
    check(onlyResult(r, "position_training_result"), "Position Select: a successful answer carries only the position result");
  }
  // Learning the position the other slot holds is offered by the client: one more level.
  {
    ClientCard c = shown(721);
    check(clientOffers(c, 1, 10), "the client offers RWG for slot 1 while slot 0 holds it at level 1");
    auto r = pick(R, 721, 1, 10)->getJSONObject("position_training_result");
    check(r->getString("is_success") == "YES" && shown(721).level[10] == 2, "Position Select: RWG in both slots is level 2");
    pick(R, 721, 0, 9);  // LWG (1) replaces RWG in slot 0: RWG back to 1, LWG 2
    c = shown(721);
    check(c.level[10] == 1 && c.level[9] == 2 && c.slot[0] == 9 && c.slot[1] == 10, ("Position Select: replacing one of the two goes one level down, " + describe(c)).c_str());
  }
  // (S has no record.) DMF at its own 1 is learned at 2; when OMF replaces it, DMF is back to 1.
  {
    pick(S, 722, 0, 4);
    check(shown(722).level[4] == 2, "Position Select: DMF at the card's own level 1 is learned at level 2");
    pick(S, 722, 0, 8);
    ClientCard c = shown(722);
    check(c.level[4] == 1 && c.level[8] == 2, ("Position Select: a card without a record gets its own level back when a position is replaced, " + describe(c)).c_str());
  }
  // What the client does not offer is refused and changes nothing: CB is neither played by R nor next to ST.
  {
    auto before = saveDump(s);
    auto r = pick(R, 721, 0, 1);
    check(r->getJSONObject("position_training_result")->getString("is_success") == "NO" && firstDifference(before, saveDump(s)).empty() &&
          onlyResult(r, "position_training_result"), "Position Select: a position the client does not offer is refused and nothing changes");
  }
  // A goalkeeper learns nothing.
  for (int position : {12, 1}) {
    auto before = saveDump(s);
    auto a = pick(G, 725, 0, position), b = use("POSITION_TRAINING", G, 725, 0, 0, newObj());
    check(a->getJSONObject("position_training_result")->getString("is_success") == "NO" && b->getJSONObject("position_training_result")->getString("is_success") == "NO" &&
          firstDifference(before, saveDump(s)).empty(), "Position Training: a goalkeeper learns no position");
  }
  // Any sequence of picks and random trainings: after every answer the next login shows exactly
  // what the client showed after that answer. Picks the client does not offer are refused.
  for (long long serial : {721LL, 722LL, 723LL, 724LL}) {
    long long id = bySerial(login(), serial)->getJSONObject("gameplayer_id")->getLong("id");
    srand((unsigned)serial);
    for (int step = 0; step < 60; ++step) {
      ClientCard c = shown(serial);
      int slot = c.slot[0] == 13 ? 0 : rand() % 2;  // the client opens slot 1 only once slot 0 holds a position
      std::string what = "card " + std::to_string(serial) + " step " + std::to_string(step) + " slot " + std::to_string(slot);
      if (rand() % 2) {
        int position = rand() % 13;
        auto r = pick(id, serial, slot, position)->getJSONObject("position_training_result");
        if (clientOffers(c, slot, position)) {
          check(r->getString("is_success") == "YES" && r->getString("additional_position") == kPositionNames[position], (what + ": the pick is learned").c_str());
          clientLearns(c, slot, position);
        } else {
          check(r->getString("is_success") == "NO", (what + ": a pick the client does not offer is refused").c_str());
        }
      } else {
        auto r = use("POSITION_TRAINING", id, serial, slot, 0, newObj())->getJSONObject("position_training_result");
        int got = positionNumber(r->getString("additional_position"));
        bool any = false;
        for (int p = 0; p < 13; ++p) any |= clientOffers(c, slot, p);
        check(any ? (r->getString("is_success") == "YES" && got >= 0 && clientOffers(c, slot, got)) : r->getString("is_success") == "NO",
            (what + ": random training teaches a position the client offers, got " + r->getString("additional_position")).c_str());
        if (any) clientLearns(c, slot, got);
      }
      ClientCard after = shown(serial);
      check(sameCard(after, c), (what + ": the login shows " + describe(after) + ", the client showed " + describe(c)).c_str());
    }
  }
  // The save keeps it: a new start shows the same cards.
  {
    std::vector<ClientCard> before;
    for (long long serial : {721, 722, 723, 724}) before.push_back(shown(serial));
    OfflineServer again(cfg);
    auto list = call(again, "CMD_LOGIN")->getJSONObject("entry_info")->getJSONArray("gameplayer_list");
    int i = 0;
    for (long long serial : {721, 722, 723, 724}) check(sameCard(clientCard(bySerial(list, serial)), before[i++]), "a new start shows the trained cards as before");
  }
  // Each answer carries only its own result. A booster answer also carries the card's sell value.
  {
    auto booster = use("ABILITY_TRAINING_SELECT", S, 722, 0, 0, newObj()->put("ability_id", 186));
    check(booster->getJSONObject("ability_training_result")->getString("is_success") == "YES" && onlyResult(booster, "ability_training_result") &&
          booster->optLong("gameplayer_sell_gp", -1) == 150000, ("Booster: only the booster result, and the card's sell value, " + booster->toString()).c_str());
    booster = use("ABILITY_TRAINING_RANDOM", R, 721, 1, 0, newObj());
    check(booster->getJSONObject("ability_training_result")->getString("is_success") == "YES" && onlyResult(booster, "ability_training_result") &&
          booster->optLong("gameplayer_sell_gp", -1) == 230000, "Booster (random): only the booster result, and the card's sell value");
    auto skill = use("SKILL_TRAINING", R, 721, 0, 0, newObj());
    check(skill->getJSONObject("skill_training_result")->getString("is_success") == "YES" && onlyResult(skill, "skill_training_result"),
        "Skill training: only the skill result");
    auto level = use("LEVEL_TRAINING", R, 721, 0, 1, newObj());
    check(onlyResult(level, ""), "Level training: no training result");
  }
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
      // Grouped writes: an inner group is undone on its own, a match result inside an undone
      // group is undone with it, and the outer group's writes stay.
      {
        std::string v;
        s.ziruDb->begin();
        s.ziruDb->saveMeta("group_outer", "1");
        s.ziruDb->begin();
        s.ziruDb->saveMeta("group_inner", "1");
        s.ziruDb->rollback();
        s.ziruDb->begin();
        s.ziruDb->saveMatchState(R"({"division":10})", "group_result", "1");
        s.ziruDb->rollback();
        s.ziruDb->commit();
        check(s.ziruDb->loadMeta("group_outer", v) && !s.ziruDb->loadMeta("group_inner", v) && !s.ziruDb->loadMeta("group_result", v) &&
              s.ziruDb->txDepth == 0, "grouped writes: inner groups are undone alone, the outer group is written");
        { OfflineServer::ZiruTx tx(*s.ziruDb); s.ziruDb->saveMeta("group_dropped", "1"); }
        check(!s.ziruDb->loadMeta("group_dropped", v) && s.ziruDb->txDepth == 0, "grouped writes: a group left without commit is undone");
      }
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
    serialTests(root);
    starterSerialTests(root);
    selectTrainingTests(root);
    positionTrainingTests(root);
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
    std::cout << "PASS: gameplay, booster layout, all 72 skill mappings/roundtrip/training/skill-zero restart, CREATE_USER resume, D1 migration, failed writes and abrupt-exit persistence, card identity, old-save repair, match-start parameters and outgoing style check\n";
    return 0;
  } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << "\n"; return 1; }
}
