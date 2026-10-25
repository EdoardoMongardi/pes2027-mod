// ---- z27: division league / squads / kits (OfflineServer.java ~3801-4682) ----

ObjP z27Meta(const std::string& k) {
  JStr m = loadMeta(k);
  if (!m || m->empty()) return nullptr;
  return Object::parse(*m);
}

int32_t z27Int(const std::string& k, int32_t def) {
  try {
    JStr m = loadMeta(k);
    return !m ? def : parseInt(*m);
  } catch (const std::exception&) {
    return def;
  }
}

ObjP z27Request() {
  ObjP r = decodeZiruRequestObject(ziruLastRequestBody);
  if (!r) throw IllegalState("Request decode failed; no commit");
  return r;
}

ObjP z27Ack(const std::string& msg, const ObjP& req) {
  return newObj()->put("msgid", msg)->put("rqid", (long long)nn(req)->optLong("rqid", 1))->put("result", "NOERR")->put("errcode", "NOERR");
}

Response z27Error(const std::string& msg, const std::exception& e) {
  logRequest("ZIRU_MATCH_ERROR", msg + ": " + e.what());
  ObjP req = decodeZiruRequestObject(ziruLastRequestBody);
  return serveObjectAsEncryptedMsgPack(
      newObj()->put("msgid", msg)->put("rqid", (long long)(req ? req->optLong("rqid", 1) : 1))->put("result", "ERROR")->put("errcode", "ERROR"));
}

ObjP z27Teams() { return nn(loadZiruJsonAssetObject("responses/ZIRU_MATCH_TEAMS.json"))->getJSONObject("teams"); }

ObjP z27Team(int32_t id) {
  if (id == 262143) {
    ObjP syn = newObj()
        ->put("base_team_id", 262143)
        ->put("base_team_license", "UNKNOWN")
        ->put("team_name", "PES Team")
        ->put("team_name_short", "PES");
    ArrP kits = newArr();
    kits->put(newObj()->put("id", 1)->put("shirt_rgb", newArr()->put(255)->put(255)->put(255)));
    kits->put(newObj()->put("id", 2)->put("shirt_rgb", newArr()->put(0)->put(0)->put(0)));
    syn->put("kits", kits);
    return syn;
  }
  ObjP teams = z27Teams();
  int32_t lookup_id = (id <= 0) ? 100 : id;
  ObjP t = teams ? teams->optJSONObject(str(lookup_id)) : nullptr;
  if (!t && teams) {
    for (const char* tid : {"100", "109", "102", "173", "135"}) {
      t = teams->optJSONObject(tid);
      if (t) break;
    }
    if (!t) {
      for (const auto& k : teams->keys()) {
        t = teams->optJSONObject(k);
        if (t) break;
      }
    }
  }
  if (!t) {
    std::string lic = (lookup_id == 262143) ? "UNKNOWN" : "LICENSED";
    ObjP syn = newObj()
        ->put("base_team_id", lookup_id)
        ->put("base_team_license", lic)
        ->put("team_name", "PES Team")
        ->put("team_name_short", "PES");
    ArrP kits = newArr();
    kits->put(newObj()->put("id", 1)->put("shirt_rgb", newArr()->put(255)->put(255)->put(255)));
    kits->put(newObj()->put("id", 2)->put("shirt_rgb", newArr()->put(0)->put(0)->put(0)));
    syn->put("kits", kits);
    return syn;
  }
  ObjP copy = z27Copy(t);
  return copy;
}

ObjP z27Identity(const ObjP& t) {
  int32_t tid = nn(t)->getInt("base_team_id");
  std::string lic = (tid == 262143) ? "UNKNOWN" : "LICENSED";
  return newObj()
      ->put("base_team_id", tid)
      ->put("base_team_license", lic)
      ->put("team_name", t->getString("team_name"))
      ->put("team_name_short", t->getString("team_name_short"));
}

inline void z27InjectTrainingItems(const ObjP& entry) {
  if (!entry) return;
  ArrP items = entry->optJSONArray("item_list");
  if (!items) {
    items = newArr();
    entry->put("item_list", items);
  }
  std::set<std::string> existing;
  for (int i = 0; i < items->length(); i++) {
    ObjP it = items->optJSONObject(i);
    ObjP item = it ? it->optJSONObject("item") : nullptr;
    if (item) {
      existing.insert(item->optString("item_type", ""));
      it->put("inventory_num", 999999);
    }
  }
  const char* needed[] = {
      "SKILL_TRAINING", "SKILL_TRAINING_CATEGORY", "SKILL_TRAINING_SELECT",
      "POSITION_TRAINING", "POSITION_TRAINING_SELECT",
      "ABILITY_TRAINING_SELECT", "ABILITY_TRAINING_RANDOM",
      "LEVEL_TRAINING", "EXTEND_PLAYER"
  };
  for (const char* t : needed) {
    if (!existing.count(t)) {
      ObjP selItem = newObj();
      ObjP inner = newObj()->put("item_type", t)->put("grade", 0)->put("effect", 0);
      selItem->put("item", inner)->put("inventory_num", 999999);
      items->put(selItem);
    }
  }
}

inline void z27BoostCoach(const ObjP& c) {
  if (!c) return;
  c->put("possession", 99);
  c->put("short_counter", 99);
  c->put("long_ball", 99);
  c->put("long_counter", 99);
  c->put("side_attack", 99);
  c->put("overload", 99);
}

inline void z27ActivateCoachLink(const ObjP& sqData) {
  if (!sqData) return;
  ObjP gp = sqData->optJSONObject("squad_gameplan");
  ArrP forms = gp ? gp->optJSONArray("formation") : nullptr;
  if (!forms) return;
  for (int i = 0; i < forms->length(); i++) {
    ObjP f = forms->optJSONObject(i);
    if (!f) continue;
    ArrP cld = f->optJSONArray("coach_link_data");
    if (!cld || cld->length() == 0) {
      cld = newArr();
      cld->put(newObj()->put("is_link_on", "YES")->put("key_member_Id", 0)->put("link_member_Id", 1));
      cld->put(newObj()->put("is_link_on", "YES")->put("key_member_Id", 2)->put("link_member_Id", 3));
      f->put("coach_link_data", cld);
    } else {
      for (int k = 0; k < cld->length(); k++) {
        ObjP link = cld->optJSONObject(k);
        if (link) {
          link->put("is_link_on", "YES");
          if (link->optInt("key_member_Id", 255) == 255) link->put("key_member_Id", k * 2);
          if (link->optInt("link_member_Id", 255) == 255) link->put("link_member_Id", k * 2 + 1);
        }
      }
    }
  }
}

inline void z27BoostAllCoaches(const ObjP& entry) {
  if (!entry) return;
  ArrP cl = entry->optJSONArray("coach_list");
  if (cl) {
    for (int i = 0; i < cl->length(); i++) {
      z27BoostCoach(cl->optJSONObject(i));
    }
  }
  ArrP sqs = entry->optJSONArray("squad_list");
  if (sqs) {
    for (int i = 0; i < sqs->length(); i++) {
      ObjP sq = sqs->optJSONObject(i);
      ObjP sd = sq ? sq->optJSONObject("squad_data") : nullptr;
      if (sd) {
        z27ActivateCoachLink(sd);
        ArrP co = sd->optJSONArray("coach");
        if (co) {
          for (int j = 0; j < co->length(); j++) {
            z27BoostCoach(co->optJSONObject(j));
          }
        }
      }
    }
  }
}

// ---- The starter cards (CmdLogin.json), read once per process ----
// entry_info as the client gets it (training items, coach settings) and an
// index of its cards. Nothing in here is ever changed: whoever hands a card or
// the entry out, or changes it, takes a copy first.
struct ZiruStarters {
  bool loaded = false;
  ObjP entry;
  // (serial, id) -> listed card; also under the id the iOS client knows the card by
  // (z27UpgradeIosCardPid, e.g. 40352 -> 89133993205152)
  std::map<std::pair<int64_t, int64_t>, ObjP> bySerialId;
  std::unordered_map<int64_t, ObjP> byId;          // id -> first card listed with it
  std::unordered_map<int64_t, ObjP> byUpgradedId;  // upgraded id -> first card listed with the old id
  std::set<int64_t> serials;                       // every serial a starter card is listed under
};
ZiruStarters ziruStarters;
int ziruLoginParses = 0;  // how often CmdLogin.json was parsed (checked by the tests)

const ZiruStarters& z27Starters() {
  if (ziruStarters.loaded) return ziruStarters;
  ziruStarters.loaded = true;
  ObjP login = loadZiruJsonAssetObject("responses/CmdLogin.json");
  ObjP e = login ? login->optJSONObject("entry_info") : nullptr;
  if (!e) return ziruStarters;
  z27InjectTrainingItems(e);
  z27BoostAllCoaches(e);
  ziruStarters.entry = e;
  ArrP list = e->optJSONArray("gameplayer_list");
  for (int i = 0; list && i < list->length(); i++) {
    ObjP card = list->optJSONObject(i);
    ObjP gid = card ? card->optJSONObject("gameplayer_id") : nullptr;
    if (!gid) continue;
    int64_t id = gid->optLong("id", 0), serial = gid->optLong("serial", 0);
    int64_t upgraded = z27UpgradeIosCardPid(id, card->optInt("initial_base_team_id", 0));
    ziruStarters.bySerialId.emplace(std::make_pair(serial, id), card);
    ziruStarters.byId.emplace(id, card);
    ziruStarters.serials.insert(serial);
    if (upgraded != id) {
      ziruStarters.bySerialId.emplace(std::make_pair(serial, upgraded), card);
      ziruStarters.byUpgradedId.emplace(upgraded, card);
    }
  }
  return ziruStarters;
}

ObjP z27Entry() { return z27Clone(nn(z27Starters().entry)); }

int32_t z27Main() { return z27Int("main_squad_id", nn(z27Starters().entry)->getInt("main_squad_id")); }

// The two Standard cards of Konami's captured starter squad that the iOS client knows by another card: the Neymar
// 40352 as the Big Time Neymar 89133993205152 (88029918174624 with team 1254) and the De Bruyne 44379 as the Show Time
// De Bruyne 106788187843931. This applies to starter cards only (z28ShownCardId).
inline int64_t z27UpgradeStarterPid(int64_t pid, int32_t teamId) {
  int32_t bid = (int32_t)(pid & 0xFFFFFF);
  if (bid == 40352 && pid <= 0xFFFFFF) {
    return (teamId == 1254) ? 88029918174624LL : 89133993205152LL;
  }
  if (bid == 44379 && pid <= 0xFFFFFF) {
    return 106788187843931LL;
  }
  return pid;
}

// The id the iOS client knows a card by: the starter upgrade above, or for the cards below (Casemiro, Eriksen,
// Mkhitaryan, Calhanoglu, Fati) another card of the same player whose card background the client has.
inline int64_t z27UpgradeIosCardPid(int64_t pid, int32_t teamId = 0) {
  int32_t bid = (int32_t)(pid & 0xFFFFFF);
  int64_t starter = z27UpgradeStarterPid(pid, teamId);
  if (starter != pid) return starter;
  if ((bid == 42669 || bid == 100780) && (pid == 89131308852909LL || pid <= 0xFFFFFF)) {
    return 89132651030189LL;
  }
  if ((bid == 40871 || bid == 42659) && (pid == 89131308851111LL || pid <= 0xFFFFFF)) {
    return 89132651028391LL;
  }
  if ((bid == 39936 || bid == 43026) && (pid == 89131308850176LL || pid <= 0xFFFFFF)) {
    return 89132382592000LL;
  }
  if ((bid == 101097 || bid == 105809) && (pid == 89131308911337LL || pid <= 0xFFFFFF)) {
    return 89132382653161LL;
  }
  if ((bid == 133215 || bid == 135760) && (pid == 89129966766175LL || pid <= 0xFFFFFF)) {
    return 105628546762847LL;
  }
  return pid;
}

// The id the client knows a card held under `serial` by. The starter upgrade applies only to a starter card (held
// under the serial CmdLogin.json lists it under): a Standard Neymar 40352 drawn from a box stays 40352 and never
// becomes the BIG TIME box's Neymar. The card background upgrades apply to every card, as before.
int64_t z28ShownCardId(int64_t pid, int32_t teamId, int64_t serial) {
  if (!z28IsStarterSerial(serial) && z27UpgradeStarterPid(pid, teamId) != pid) return pid;
  return z27UpgradeIosCardPid(pid, teamId);
}

ObjP z27Squad(int32_t id) {
  JStr s = loadSquad(id);
  ObjP res = nullptr;
  if (s) {
    ObjP jo = Object::parse(*s);
    res = jo->optJSONObject("squad_data") ? jo->getJSONObject("squad_data") : jo;
  } else {
    ArrP list = nn(z27Starters().entry)->getJSONArray("squad_list");
    for (int i = 0; i < list->length(); i++) {
      ObjP sq = list->getJSONObject(i);
      if (sq->optInt("squad_id", -1) == id) { res = z27Copy(sq->getJSONObject("squad_data")); break; }
    }
    if (!res && list->length() > 0) res = z27Copy(list->getJSONObject(0)->getJSONObject("squad_data"));
    // The starter squad lists the starter cards: a club without them has no one in its squad yet.
    if (res && z28NoStarters()) res->put("player_list", newArr());
  }
  if (!res) throw IllegalState("Unknown squad " + str(id));
  ArrP pl = res->optJSONArray("player_list");
  if (pl) {
    bool mod = false;
    for (int i = 0; i < pl->length(); i++) {
      ObjP item = pl->optJSONObject(i);
      ObjP gid = item ? item->optJSONObject("gameplayer_id") : nullptr;
      if (gid) {
        int64_t pid = gid->optLong("id", 0);
        int team = item ? item->optInt("initial_base_team_id", 0) : 0;
        int64_t upPid = z28ShownCardId(pid, team, gid->optLong("serial", 0));
        if (upPid != pid) {
          gid->put("id", upPid);
          mod = true;
        }
      }
    }
    if (mod && s) {
      ziruDb->saveSquad(id, res->toString());
    }
  }
  return res;
}

ObjP z27Base(int32_t id) {
  ObjP m = z27Meta("z27_base_" + str(id));
  if (!m) m = z27Meta("ziru_base_team_squad_" + str(id));
  if (m) {
    ObjP ident = z27Identity(z27Team(m->getInt("base_team_id")));
    std::string defName = z27Team(m->getInt("base_team_id"))->getString("team_name");
    ident->put("team_name", m->optString("team_name", defName));
    std::string defShort = z27Team(m->getInt("base_team_id"))->getString("team_name_short");
    ident->put("team_name_short", m->optString("team_name_short", defShort));
    ident->put("base_team_license", z27Team(m->getInt("base_team_id"))->optString("base_team_license", "LICENSED"));
    return ident;
  }
  ArrP list = nn(z27Starters().entry)->getJSONArray("squad_list");
  for (int i = 0; i < list->length(); i++) {
    ObjP sq = list->getJSONObject(i);
    if (sq->optInt("squad_id", -1) == id) return z27Identity(z27Team(sq->getJSONArray("base_team")->getJSONObject(0)->getInt("base_team_id")));
  }
  if (list->length() > 0) return z27Identity(z27Team(list->getJSONObject(0)->getJSONArray("base_team")->getJSONObject(0)->getInt("base_team_id")));
  return z27Identity(z27Team(100));
}

ArrP z27Kits(const ObjP& team) {
  ArrP k = team->getJSONArray("kits");
  ArrP out = newArr();
  for (int i = 0; i < k->length(); i++) out->put(k->getJSONObject(i)->getInt("id"));
  return out;
}

ObjP z27Uniform(const ObjP& base, const ArrP& sel) {
  ArrP kits = z27Kits(z27Team(base->getInt("base_team_id")));
  ArrP out;
  if (sel) {
    out = newArr();
    for (int i = 0; i < sel->length(); i++) {
      out->put(sel->getInt(i));
    }
    for (int j = 0; j < kits->length(); j++) {
      bool have = false;
      for (int i = 0; i < out->length(); i++) if (out->getInt(i) == kits->getInt(j)) have = true;
      if (!have) out->put(kits->getInt(j));
    }
  } else {
    out = kits;
  }
  return newObj()->put("base_team_id", base->getInt("base_team_id"))->put("uniform_list", out);
}

ObjP z27FindBase(const ObjP& jo) {
  ObjP o = jo->optJSONObject("base_team");
  if (o) return o;
  ArrP a = jo->optJSONArray("base_team");
  if (a && a->optJSONObject(0)) return a->optJSONObject(0);
  if (jo->has("base_team_id")) return jo;
  for (const char* k : {"entry_info", "squad_info", "team_data"}) {
    ObjP c = jo->optJSONObject(k);
    ObjP f;
    if (c && (f = z27FindBase(c))) return f;
  }
  return nullptr;
}

Response z27TeamChange(const std::string& cmd) {
  try {
    ObjP req = z27Request();
    int sq = req->optInt("squad_id", z27Main());
    if (cmd == "CMD_SET_MYCLUB_MAIN_SQUAD") {
      sq = req->optInt("main_squad_id", sq);
      z27Squad(sq);
      z27Base(sq);
      z27PutMeta("main_squad_id", str(sq));
    } else if (cmd == "CMD_SET_MYCLUB_BRING_UNIFORM") {
      ArrP sel = req->optJSONArray("set_uniform_list");
      if (!sel) sel = req->optJSONArray("uniform_list");
      ObjP ul;
      if (!sel && (ul = req->optJSONObject("uniform_list"))) sel = ul->optJSONArray("uniform_list");
      if (!sel) throw IllegalArg("Missing uniform selection");
      z27Uniform(z27Base(sq), sel);
      z27PutMeta("z27_kits_" + str(sq), sel->toString());
    } else {
      ObjP fb = z27FindBase(req);
      if (!fb && cmd == "CMD_SET_MYCLUB_ENTRY_INFO") {
        fb = z27Copy(req);
        int defTid = 100;
        try { defTid = z27Base(sq)->getInt("base_team_id"); } catch (...) {}
        fb->put("base_team_id", defTid);
      }
      if (!fb) {
        fb = newObj()->put("base_team_id", 100);
      }
      int tid = fb->getInt("base_team_id");
      if (tid <= 0) {
        tid = 100;
        fb->put("base_team_id", tid);
      }
      ObjP ident = z27Identity(z27Team(tid));
      if (cmd == "CMD_SET_MYCLUB_ENTRY_INFO") {
        if (fb->has("team_name")) ident->put("team_name", fb->getString("team_name"));
        if (fb->has("team_name_short")) ident->put("team_name_short", fb->getString("team_name_short"));
      }
      z27PutMeta("z27_base_" + str(sq), ident->toString());
      z27PutMeta("z27_kits_" + str(sq), z27Kits(z27Team(ident->getInt("base_team_id")))->toString());
    }
    if (cmd == "CMD_SET_MYCLUB_ENTRY_INFO") {
      ObjP ei = z27Entry();
      ObjP curBase = z27Base(sq);
      ei->put("team_name", curBase->getString("team_name"));
      ei->put("team_name_short", curBase->getString("team_name_short"));
      ei->put("base_team_id", curBase->getInt("base_team_id"));
      ei->put("base_team_license", curBase->optString("base_team_license", "LICENSED"));
      ObjP ack = z27Ack(cmd, req);
      ack->put("base_team", newArr()->put(curBase));
      ack->put("entry_info", ei);
      ZiruTx tx(*ziruDb);
      z27Restore(ack);
      tx.commit();
      logRequest("ZIRU_TEAM_COMMIT", "squad=" + str(sq) + " base=" + str(curBase->getInt("base_team_id")));
      return serveObjectAsEncryptedMsgPack(ack);
    }
    ObjP ack = z27Ack(cmd, req);
    ack->put("base_team", newArr()->put(z27Base(sq)));
    ack->put("entry_info", z27Entry());
    ZiruTx tx(*ziruDb);
    z27Restore(ack);
    tx.commit();
    ack->put("main_squad_id", z27Main());
    logRequest("ZIRU_TEAM_COMMIT", "squad=" + str(sq) + " base=" + str(z27Base(sq)->getInt("base_team_id")));
    return serveObjectAsEncryptedMsgPack(ack);
  } catch (const std::exception& e) {
    return z27Error(cmd, e);
  }
}

void z27SaveSquad(const std::optional<Bytes>& body) {
  ObjP req = decodeZiruRequestObject(body);
  if (!req) throw IllegalArg("No squad request");
  int sq = req->optInt("squad_id", z27Main());
  ObjP data = req->optJSONObject("squad_data");
  if (!data) throw IllegalArg("No squad_data");
  ObjP cur = z27Squad(sq);
  for (auto& k : data->keys()) cur->put(k, data->get(k));
  ArrP curCoach = cur->optJSONArray("coach");
  if (!curCoach || curCoach->length() == 0) {
    cur->put("coach", newArr()->put(newObj()->put("coach_id", newObj()->put("id", 17608292273375LL)->put("serial", 359042717000000009LL))));
  }
  ObjP fb = z27FindBase(req);
  if (fb) z27Team(fb->getInt("base_team_id"));
  std::string s = cur->toString();
  ziruDb->saveSquad(sq, s);
  JStr back = loadSquad(sq);
  if (!back || *back != s) throw IllegalState("SQLite squad save failed");
  if (fb) {
    ObjP ident = z27Identity(z27Team(fb->getInt("base_team_id")));
    if (fb->has("team_name")) ident->put("team_name", fb->getString("team_name"));
    if (fb->has("team_name_short")) ident->put("team_name_short", fb->getString("team_name_short"));
    bool changed = z27Base(sq)->getInt("base_team_id") != fb->getInt("base_team_id");
    z27PutMeta("z27_base_" + str(sq), ident->toString());
    if (changed) z27PutMeta("z27_kits_" + str(sq), z27Kits(z27Team(fb->getInt("base_team_id")))->toString());
  }
  ArrP sel = req->optJSONArray("set_uniform_list");
  if (sel) {
    z27Uniform(z27Base(sq), sel);
    z27PutMeta("z27_kits_" + str(sq), sel->toString());
  }
  logRequest("ZIRU_SQUAD_COMMIT", "squad=" + str(sq));
}

Response z27SquadAck() {
  try {
    ObjP req = z27Request();
    z27SaveSquad(ziruLastRequestBody);
    return serveObjectAsEncryptedMsgPack(z27Ack("CMD_SET_MYCLUB_SQUAD_INFO", req)->put("achievement_notification", newArr())->put("user_stepup_list", newArr()));
  } catch (const std::exception& e) {
    return z27Error("CMD_SET_MYCLUB_SQUAD_INFO", e);
  }
}

std::set<int64_t> ziruRowOtherCardLogged;  // serials already reported, so each is logged once

// A card that is not saved yet, as its row will hold it: the skills learned for
// this serial, then the ones its entry carries (repaired, at most 5), switched on
// in its parameters, and the usual normalisation. Returns the learned list as
// it is kept in prefs ("" when there is none).
std::string z27PrepareFreshCard(const ObjP& p, int64_t serial) {
  std::vector<int32_t> skillList;
  std::string prefSkills = sp("almanac_state").getString("ziru_skills_" + str(serial), "");
  if (!prefSkills.empty()) {
    for (auto& part : split(prefSkills, ',')) {
      try {
        int val = ziruRepairLearnedSkill(parseInt(part));
        if (isZiruSkillId(val) && !vcontains(skillList, val) && skillList.size() < 5) skillList.push_back(val);
      } catch (const NumberFormat&) {}
    }
  }
  ArrP existingSkills = p->optJSONArray("additional_skill_id");
  for (int sIdx = 0; existingSkills && sIdx < existingSkills->length(); ++sIdx) {
    int val = ziruRepairLearnedSkill(existingSkills->optInt(sIdx));
    if (isZiruSkillId(val) && !vcontains(skillList, val) && skillList.size() < 5) skillList.push_back(val);
  }
  ArrP newSkillArr = newArr();
  for (int v : skillList) newSkillArr->put(v);
  p->put("additional_skill_id", newSkillArr);
  ArrP pParams = p->optJSONArray("parameters");
  if (pParams && pParams->length() >= 142) {
    for (int v : skillList) {
      int pIdx = kZiruSkillParameters[v];
      if (isZiruSkillParameter(pIdx) && pIdx < pParams->length()) pParams->put(pIdx, 1);
    }
  }
  z27NormalizeOwnedCard(p);
  return joinInts(skillList);
}

void z27Restore(const ObjP& jo) {
  try {
    ObjP ei = jo->optJSONObject("entry_info");
    if (!ei) return;
    ArrP squads = ei->optJSONArray("squad_list");
    ObjP target;
    if (!squads) {
      ObjP e = z27Entry();
      jo->put("entry_info", e);
      squads = e->getJSONArray("squad_list");
      target = e;
    } else {
      target = ei;
    }
    ObjP base = z27Base(z27Main());
    for (auto& k : base->keys()) target->put(k, base->get(k));
    int mainId = z27Main();
    bool found = false;
    for (int i = 0; i < squads->length(); i++) {
      ObjP s = squads->getJSONObject(i);
      int id = s->getInt("squad_id");
      if (id == mainId) found = true;
      if (loadSquad(id) || z28NoStarters()) {
        ObjP sqData = z27Squad(id);
        ArrP coachArr = sqData->optJSONArray("coach");
        if (!coachArr || coachArr->length() == 0) {
          sqData->put("coach", newArr()->put(newObj()->put("coach_id", newObj()->put("id", 17608292273375LL)->put("serial", 359042717000000009LL))));
          ziruDb->saveSquad(id, sqData->toString());
        }
        s->put("squad_data", sqData);
      }
      s->put("base_team", newArr()->put(z27Base(id)));
    }
    if (!found) {
      ObjP sqData = z27Squad(mainId);
      ArrP coachArr = sqData->optJSONArray("coach");
      if (!coachArr || coachArr->length() == 0) {
        sqData->put("coach", newArr()->put(newObj()->put("coach_id", newObj()->put("id", 17608292273375LL)->put("serial", 359042717000000009LL))));
        ziruDb->saveSquad(mainId, sqData->toString());
      }
      squads->put(newObj()->put("squad_id", mainId)->put("name", "Dream Team")->put("squad_data", sqData)->put("base_team", newArr()->put(z27Base(mainId))));
    }
    std::set<int> present;
    for (int i = 0; i < squads->length(); i++) present.insert(squads->getJSONObject(i)->getInt("squad_id"));
    for (int id = 1; id <= 20; id++) {
      if (present.find(id) == present.end() && loadSquad(id)) {
        ObjP sq = z27Squad(id);
        ArrP coachArr = sq->optJSONArray("coach");
        if (!coachArr || coachArr->length() == 0) {
          sq->put("coach", newArr()->put(newObj()->put("coach_id", newObj()->put("id", 17608292273375LL)->put("serial", 359042717000000009LL))));
          ziruDb->saveSquad(id, sq->toString());
        }
        std::string sname = sq->optString("squad_name", "Tactics " + str(id));
        squads->put(newObj()->put("squad_id", id)->put("name", sname)->put("squad_data", sq)->put("base_team", newArr()->put(z27Base(id))));
      }
    }
    target->put("main_squad_id", mainId);
    z27InjectTrainingItems(target);
    z27BoostAllCoaches(target);
    target->put("current_gameplayer_warehouse_list_max", 100000);
    target->put("warehouse_player_max", 100000);
    target->put("current_gameplayer_list_max", 100000);
    target->put("gameplayer_list_max", 100000);
    target->put("unlock_gameplayer_frame", 100000);
    target->put("unlock_gameplayer_warehouse_frame", 100000);
    JStr kits = loadMeta("z27_kits_" + str(mainId));
    target->put("set_uniform_list", z27Uniform(z27Base(mainId), kits ? Array::parse(*kits) : nullptr)->getJSONArray("uniform_list"));
    ArrP gl = target->optJSONArray("gameplayer_list");
    if (gl) {
      ArrP kept = newArr();
      for (int i = 0; i < gl->length(); i++) {
        ObjP p = gl->getJSONObject(i);
        int64_t listedSerial = p->getJSONObject("gameplayer_id")->getLong("serial");
        if (z28NoStarters() && z28IsStarterSerial(listedSerial)) continue;  // a club that started with no players
        if (!isZiruPlayerSold(listedSerial)) kept->put(p);
      }
      target->put("gameplayer_list", kept);
      ArrP all = Array::parse(ziruDb->loadAllPlayersJson());
      std::set<int64_t> have;
      for (int i = 0; i < kept->length(); i++) have.insert(kept->getJSONObject(i)->getJSONObject("gameplayer_id")->optLong("serial", 0));
      for (int i = 0; i < all->length(); i++) {
        ObjP p = all->getJSONObject(i);
        int64_t s = p->getJSONObject("gameplayer_id")->getLong("serial");
        if (!isZiruPlayerSold(s) && !have.count(s)) kept->put(p);
      }
      gl = kept;
    }
    if (gl) {
      for (int i = 0; i < gl->length(); i++) {
        ObjP p = gl->getJSONObject(i);
        int64_t serial = p->getJSONObject("gameplayer_id")->optLong("serial", 0);
        int64_t pPid = p->getJSONObject("gameplayer_id")->getLong("id");
        int team = p->optInt("initial_base_team_id", 0);
        int64_t upPid = z28ShownCardId(pPid, team, serial);
        if (upPid != pPid) {
          pPid = upPid;
          p->getJSONObject("gameplayer_id")->put("id", pPid);
        }
        JStr lp = loadPlayer(serial);
        if (lp) {
          ObjP sv = Object::parse(*lp);
          if (sv->optJSONObject("gameplayer")) sv = sv->getJSONObject("gameplayer");
          int64_t svPid = sv->getJSONObject("gameplayer_id")->getLong("id");
          int svTeam = sv->optInt("initial_base_team_id", 0);
          int64_t upSvPid = z28ShownCardId(svPid, svTeam, serial);
          if (upSvPid != svPid) {
            svPid = upSvPid;
            sv->getJSONObject("gameplayer_id")->put("id", svPid);
            ziruDb->savePlayer(serial, svPid, sv->toString());
          }
          // Only the same card is restored from the save. Another version of the
          // player is another card, even though it has the same player number.
          if (svPid == pPid) {
            z27ApplyAppearanceAndClub(sv);
            ObjP w = z28RecordFor(sv->getJSONObject("gameplayer_id")->getLong("id"), serial, true);
            ArrP baseParams = (w && w->optJSONObject("gameplayer")) ? w->getJSONObject("gameplayer")->optJSONArray("parameters") : nullptr;
            ArrP svParams = sv->optJSONArray("parameters");
            if (baseParams && svParams && baseParams->length() >= 142) {
              while (svParams->length() < 145) svParams->put(0);
              for (int idx = 70; idx <= 141; idx++) {
                if (baseParams->optInt(idx, 0) == 1) svParams->put(idx, 1);
              }
            }
            // Restore authentic booster if missing
            ObjP baseGp = (w && w->optJSONObject("gameplayer")) ? w->getJSONObject("gameplayer") : nullptr;
            if (baseGp) {
              ArrP baseBooster = baseGp->optJSONArray("initial_booster_id_list");
              ArrP svBooster = sv->optJSONArray("initial_booster_id_list");
              if (!svBooster) {
                svBooster = newArr()->put(0)->put(0)->put(0);
                sv->put("initial_booster_id_list", svBooster);
              }
              while (svBooster->length() < 3) svBooster->put(0);
              if (baseBooster) {
                for (int b = 0; b < baseBooster->length() && b < 3; ++b) {
                  int bVal = baseBooster->optInt(b, 0);
                  if (bVal > 0 && svBooster->optInt(b, 0) == 0) {
                    svBooster->put(b, bVal);
                  }
                }
              }
              if (svParams) {
                while (svParams->length() < 145) svParams->put(0);
                for (int b = 0; b < 3; ++b) {
                  int bVal = svBooster->optInt(b, 0);
                  if (bVal > 0 && svParams->optInt(142 + b, 0) == 0) {
                    svParams->put(142 + b, bVal);
                  } else if (baseParams && baseParams->length() >= 145 && svParams->optInt(142 + b, 0) == 0) {
                    int bpVal = baseParams->optInt(142 + b, 0);
                    if (bpVal > 0) {
                      svParams->put(142 + b, bpVal);
                      if (svBooster->optInt(b, 0) == 0) svBooster->put(b, bpVal);
                    }
                  }
                }
              }
            }

            // Reapply and sync additional skills
            zs::Prefs& statePrefs = sp("almanac_state");
            std::string prefKey = "ziru_skills_" + str(serial);
            std::string prefSkills = statePrefs.getString(prefKey, "");
            std::vector<int32_t> skillList;
            if (!prefSkills.empty()) {
              for (auto& part : split(prefSkills, ',')) {
                try {
                  int val = ziruRepairLearnedSkill(parseInt(part));
                  if (isZiruSkillId(val) && !vcontains(skillList, val) && skillList.size() < 5) skillList.push_back(val);
                } catch (const NumberFormat&) {}
              }
            }
            ArrP existingSkills = sv->optJSONArray("additional_skill_id");
            if (existingSkills) {
              for (int sIdx = 0; sIdx < existingSkills->length(); ++sIdx) {
                int val = ziruRepairLearnedSkill(existingSkills->optInt(sIdx));

                if (isZiruSkillId(val) && !vcontains(skillList, val) && skillList.size() < 5) skillList.push_back(val);
              }
            }
            ArrP newSkillArr = newArr();
            for (int v : skillList) newSkillArr->put(v);
            sv->put("additional_skill_id", newSkillArr);
            if (!skillList.empty() && statePrefs.getString(prefKey, "") != joinInts(skillList)) {
              statePrefs.edit().putString(prefKey, joinInts(skillList)).commit();
            }
            if (svParams && svParams->length() >= 142) {
              for (int v : skillList) {
                if (isZiruSkillId(v)) {
                  int pIdx = kZiruSkillParameters[v];
                  if (isZiruSkillParameter(pIdx) && pIdx < svParams->length()) {
                    svParams->put(pIdx, 1);
                  }
                }
              }
            }

            z27NormalizeOwnedCard(sv);
            std::string text = sv->toString();
            if (text != *lp) ziruDb->savePlayer(serial, sv->getJSONObject("gameplayer_id")->getLong("id"), text);
            gl->put(i, sv);
          } else {
            // This serial holds another card. The listed card is sent as itself, with
            // nothing taken from the save, and the saved card is left as it is.
            if (ziruRowOtherCardLogged.insert(serial).second)
              logRequest("Z27_ROW_OTHER_CARD", "[Z27_ROW_OTHER_CARD] serial=" + str(serial) + " holds " + str(svPid) + ", listed " + str(pPid));
            z27NormalizeOwnedCard(p);
          }
        } else {
          // Nothing saved under this serial yet: the listed card becomes its row.
          std::string learned = z27PrepareFreshCard(p, serial);
          zs::Prefs& statePrefs = sp("almanac_state");
          std::string prefKey = "ziru_skills_" + str(serial);
          if (!learned.empty() && statePrefs.getString(prefKey, "") != learned) statePrefs.edit().putString(prefKey, learned).commit();
          ziruDb->savePlayer(serial, p->getJSONObject("gameplayer_id")->getLong("id"), p->toString());
        }
      }
    }
  } catch (const std::exception& e) {
    logRequest("ZIRU_RESTORE_ERROR", e.what());
    throw;
  }
}

ObjP z27State() {
  ObjP m = z27Meta("z27_division_state");
  if (m) {
    if (!loadMeta("z27_div10_reset_v3")) {
      m->put("division", 10);
      m->put("best", 10);
      m->put("point", 0);
      m->put("matches", 0);
      m->put("win", 0);
      m->put("draw", 0);
      m->put("lose", 0);
      ObjP active = z27Meta("z27_active_match");
      if (active) {
        active->put("division", 10)->put("cpu_level", z27Difficulty(10));
        z27PutMeta("z27_active_match", active->toString());
      }
      z27PutMeta("z27_division_state", m->toString());
      z27PutMeta("z27_div10_reset_v3", "1");
    }
    return m;
  }
  z27PutMeta("z27_div10_reset_v3", "1");
  int div = std::max(1, std::min(10, z27Int("ziru_league_division", 10)));
  return newObj()
      ->put("division", div)
      ->put("best", std::max(div, z27Int("ziru_league_best_division", div)))
      ->put("point", z27Int("ziru_league_point", 0))
      ->put("matches", z27Int("ziru_league_matches", 0))
      ->put("win", z27Int("ziru_league_win", 0))
      ->put("draw", z27Int("ziru_league_draw", 0))
      ->put("lose", z27Int("ziru_league_lose", 0))
      ->put("sequence", z27Int("ziru_league_opponent_seq", 0))
      ->put("completed", newObj());
}

std::string z27Difficulty(int d) {
  if (d >= 9) return "NORMAL";
  if (d >= 7) return "HARD";
  if (d >= 5) return "VERYHARD";
  if (d >= 3) return "HARDEST";
  return "LEGEND";
}

int32_t z27Promotion(int d) { return ((10 - d) * 2) + 6; }

void z27Info(const ObjP& jo) { z27InfoState(jo, z27State()); }

void z27InfoState(ObjP jo, const ObjP& st) {
  ObjP li = nn(jo)->optJSONObject("league_info");
  if (li) jo = li;
  int d = st->getInt("division");
  jo->put("current_division", d)
      ->put("personal_best_division", st->getInt("best"))
      ->put("current_point", st->getInt("point"))
      ->put("round_match_count", std::max(0, std::min(st->optInt("matches"), 10)))
      ->put("point_promoted", d == 1 ? 30 : z27Promotion(d))
      ->put("point_staying", std::max(0, z27Promotion(d) - 5))
      ->put("point_relegated", std::max(0, z27Promotion(d) - 6))
      ->put("can_skip", "NO")
      ->put("is_com_match", "YES")
      ->put("rating", 0)
      ->put("pre_rating", 0)
      ->put("is_first_enter", "NO")
      ->put("is_first_best_division", "NO")
      ->put("simple_ranking", newArr())
      ->put("personal_best_division_all_phase", st->getInt("best"));
  ObjP cl = jo->optJSONObject("com_level_info");
  if (cl) cl->put("offense_com_level", z27Difficulty(d))->put("defense_com_level", z27Difficulty(d));
  ObjP rm = jo->optJSONObject("round_match_result");
  if (rm) {
    int m = st->getInt("matches");
    rm->put("win", st->getInt("win"))->put("draw", st->getInt("draw"))->put("lose", st->getInt("lose"))->put("ratio", m == 0 ? 0 : (st->getInt("win") * 100) / st->getInt("matches"));
  }
}

ObjP z27Active() {
  ObjP m = z27Meta("z27_active_match");
  if (!m) throw IllegalState("No selected opponent; request GET_VSCOM_OPPONENT first");
  ObjP state = z27State();
  if (!state->getJSONObject("completed")->has(str(m->getLong("game_id")))) {
    m->put("division", state->getInt("division"));
  }
  m->put("cpu_level", z27Difficulty(m->getInt("division")));
  z27PutMeta("z27_active_match", m->toString());
  return m;
}

ObjP z27Opponent(const ObjP& active) {
  ObjP tpl = loadZiruJsonAssetObject("responses/ZIRU_GET_VSCOM_OPPONENT.json");
  ObjP prof = active->getJSONObject("profile");
  ArrP ids = prof->getJSONArray("player_ids");
  ArrP players = newArr(), refs = newArr();
  for (int i = 0; i < ids->length(); i++) {
    ObjP w = getZiruPlayerWrapper(Value((long long)ids->getLong(i)), false);
    if (!w) throw IllegalState("Missing pack ID " + str(ids->getLong(i)));
    ObjP gp = z27Copy(w->getJSONObject("gameplayer"));
    ObjP gid = newObj()->put("id", (long long)ids->getLong(i))->put("serial", (long long)(800000000000000LL + (int64_t)(prof->getInt("profile_id") * 100) + i));
    gp->put("gameplayer_id", gid)->put("uniform_number", i + 1);
    z27ApplyAppearanceAndClub(gp);
    applyZiruWeakFootFix(gp);
    players->put(gp);
    refs->put(newObj()->put("gameplayer_id", z27Copy(gid))->put("uniform_number", i + 1));
  }
  ObjP sd = nn(tpl)->getJSONObject("squad_data");
  ObjP plan = sd->getJSONObject("squad_gameplan");
  ArrP form = plan->getJSONArray("formation");
  for (int i = 0; i < form->length(); i++) {
    ObjP f = form->getJSONObject(i);
    f->put("formation_offensive", Array::parse(prof->getJSONArray("coordinates")->toString()))
        ->put("formation_defensive", Array::parse(prof->getJSONArray("coordinates")->toString()))
        ->put("team_style", prof->getInt("team_style"))
        ->put("use_multi_formation", "NO");
    ArrP cl = f->optJSONArray("coach_link_data");
    if (!cl || cl->length() == 0) {
      cl = newArr();
      cl->put(newObj()->put("is_link_on", "YES")->put("key_member_Id", 0)->put("link_member_Id", 1));
      cl->put(newObj()->put("is_link_on", "YES")->put("key_member_Id", 2)->put("link_member_Id", 3));
      f->put("coach_link_data", cl);
    } else {
      for (int k = 0; k < cl->length(); k++) {
        ObjP link = cl->getJSONObject(k);
        if (link) {
          link->put("is_link_on", "YES");
          link->put("key_member_Id", k * 2);
          link->put("link_member_Id", k * 2 + 1);
        }
      }
    }
  }
  ArrP order = newArr();
  for (int i = 0; i < 40; i++) order->put(i < 23 ? i : 255);
  plan->getJSONObject("match_plan_settings")->put("order", order)->put("captain", 1)->put("set_piece_taker", Array::parse("[7,7,10,7,7,7]"));
  sd->put("player_list", refs);
  for (int i = 0; i < players->length(); i++) {
    ObjP p = players->getJSONObject(i);
    ObjP li = p->optJSONObject("level_info");
    if (li) {
      int lim = li->optInt("level_limit", li->optInt("level", 1));
      li->put("level", lim);
      li->put("exp_current_level", 0);
      li->put("exp_to_next_level", 0);
    }
  }
  tpl->put("player_list", players)
      ->put("team_data", z27Copy(active->getJSONObject("away")))
      ->put("game_id", (long long)active->getLong("game_id"))
      ->put("cpu_level", active->getString("cpu_level"))
      ->put("side", "AWAY");
  tpl->getJSONArray("user_info")->getJSONObject(0)->put("user_id", 1700000000 + prof->getInt("profile_id"))->put("user_name", active->getJSONObject("away")->getString("team_name"));
  return tpl;
}

Response z27SelectOpponent() {
  try {
    ObjP req = z27Request();
    ObjP st = z27State();
    std::string userCpu = req ? req->optString("cpu_level", "") : "";
    std::string effectiveCpu = z27Difficulty(st->getInt("division"));
    if (!userCpu.empty() && (userCpu == "VERYEASY" || userCpu == "EASY" || userCpu == "NORMAL" || userCpu == "HARD" || userCpu == "VERYHARD" || userCpu == "HARDEST" || userCpu == "LEGEND")) {
      effectiveCpu = userCpu;
    }
    ObjP m = z27Meta("z27_active_match");
    if (m) {
      if (!st->getJSONObject("completed")->has(str(m->getLong("game_id")))) m->put("division", st->getInt("division"));
      m->put("cpu_level", effectiveCpu);
      z27PutMeta("z27_active_match", m->toString());
    }
    if (!m || st->getJSONObject("completed")->has(str(m->getLong("game_id")))) {
      ArrP profs = nn(loadZiruJsonAssetObject("responses/ZIRU_MATCH_PROFILES.json"))->getJSONArray("profiles");
      ObjP prof = profs->getJSONObject(st->getInt("sequence") % profs->length());
      int64_t now = zc::currentTimeMillis();
      while (st->getJSONObject("completed")->has(str(now))) now++;
      m = newObj()
              ->put("game_id", (long long)now)
              ->put("division", st->getInt("division"))
              ->put("profile", z27Copy(prof))
              ->put("home", z27Base(z27Main()))
              ->put("away", z27Identity(z27Team(prof->getInt("base_team_id"))))
              ->put("cpu_level", effectiveCpu)
              ->put("squad_id", z27Main())
              ->put("created_at", (long long)(now / 1000));
      z27PutMeta("z27_active_match", m->toString());
    }
    ObjP opp = z27Opponent(m);
    opp->put("rqid", (long long)req->optLong("rqid", 1));
    logRequest("ZIRU_MATCH_SELECTED", "game=" + str(m->getLong("game_id")) + " division=" + str(m->getInt("division")) + " cpu=" + m->getString("cpu_level"));
    return serveObjectAsEncryptedMsgPack(opp);
  } catch (const std::exception& e) {
    return z27Error("CMD_GET_VSCOM_OPPONENT", e);
  }
}

// A MyLeague rental card's own catalogue entry (rental cards are not owned).
ObjP z27RentalCard(int64_t id) {
  ObjP list = loadZiruJsonAssetObject("responses/ZIRU_ML_RENTAL_LIST.json");
  ArrP rows = list ? list->optJSONArray("rental_player_list") : nullptr;
  for (int i = 0; rows && i < rows->length(); i++) {
    ObjP row = rows->optJSONObject(i);
    ObjP info = row ? row->optJSONObject("player_info") : nullptr;
    ObjP gid = info ? info->optJSONObject("gameplayer_id") : nullptr;
    if (gid && gid->optLong("id", 0) == id) return z27Copy(info);
  }
  return nullptr;
}

// The saved player of this serial when it is this exact card (also when the
// requested id is an old id that the saved card was upgraded from), else null.
// Learned skills are stored per serial and belong to this saved card only:
// other cards that Konami's starter data lists under the same serial never
// get them.
ObjP z27SavedCardFor(int64_t serial, int64_t id) {
  if (serial <= 0 || id <= 0) return nullptr;
  JStr lp = loadPlayer(serial);
  if (!lp || lp->empty()) return nullptr;
  ObjP p = Object::parse(*lp);
  if (p->optJSONObject("gameplayer")) p = p->getJSONObject("gameplayer");
  return z27IsCard(p, id) ? p : nullptr;
}

// True when `saved` is the card `id`, or the card this id is upgraded to.
bool z27IsCard(const ObjP& saved, int64_t id) {
  ObjP sid = saved ? saved->optJSONObject("gameplayer_id") : nullptr;
  int64_t savedId = sid ? sid->optLong("id", 0) : 0;
  if (savedId <= 0 || id <= 0) return false;
  return savedId == id || savedId == z27UpgradeIosCardPid(id, saved->optInt("initial_base_team_id", 0));
}

// The starter card listed under this serial with this id (a copy), or null.
ObjP z27StarterEntry(int64_t serial, int64_t id) {
  const ZiruStarters& starters = z27Starters();
  auto it = starters.bySerialId.find({serial, id});
  return it == starters.bySerialId.end() ? nullptr : z27Clone(it->second);
}

// The pack card recruited under this serial with this id, or null.
ObjP z27RecruitedEntry(int64_t serial, int64_t id) {
  std::string s = sp("almanac_state").getString("recruited_players", "");
  ArrP list = s.empty() ? nullptr : Array::parse(s);
  for (int i = 0; list && i < list->length(); i++) {
    ObjP o = list->optJSONObject(i);
    if (o && o->optJSONObject("gameplayer")) o = o->getJSONObject("gameplayer");
    ObjP gid = o ? o->optJSONObject("gameplayer_id") : nullptr;
    if (gid && gid->optLong("serial", 0) == serial && z27IsCard(o, id)) return o;
  }
  return nullptr;
}

// The card a training, booster, position or careerplan request may change:
// the card saved under the serial the client sent when it is exactly the card
// it named (or the card that id is upgraded to); else that card's own entry not
// saved yet (a starter card listed under that serial, or a pack card recruited
// under it). Null means the request is refused: the serial holds another card,
// is sold, is a rental's (serial 0, nothing is saved for rentals) or the card
// is unknown. Never another card.
ObjP z27TrainingTarget(int64_t serial, int64_t id) {
  if (serial <= 0 || id <= 0 || isZiruPlayerSold(serial)) return nullptr;
  if (ObjP saved = z27SavedCardFor(serial, id)) return saved;
  JStr row = loadPlayer(serial);
  if (row && !row->empty()) return nullptr;
  // A club that started with no players has no starter card: a request naming one is refused.
  ObjP entry = z28NoStarters() ? nullptr : z27StarterEntry(serial, id);
  if (!entry) entry = z27RecruitedEntry(serial, id);
  if (!entry) return nullptr;
  ObjP gid = entry->getJSONObject("gameplayer_id");
  gid->put("serial", (long long)serial);
  gid->put("id", (long long)z28ShownCardId(gid->optLong("id", 0), entry->optInt("initial_base_team_id", 0), serial));
  z27PrepareFreshCard(entry, serial);
  return entry;
}

// The player as the match uses it. Looked up by exact card id only (a starter
// card also by the id the client knows it by after z27UpgradeIosCardPid):
// the saved player, the starter entry for this serial, the card's record, a
// MyLeague rental card, the starter entry for this card. A rental card
// (serial 0) takes its rental entry before its record: the rental list shows
// the rental's own level and stats. A card none of these know is logged and
// null is returned; it is never built from another player.
ObjP z27Player(const ObjP& gid) {
  int64_t serial = gid->optLong("serial", 0);
  ObjP p = z27SavedCardFor(serial, gid->optLong("id", 0));
  bool fromSave = p != nullptr;
  if (!fromSave) {
    int64_t id = gid->getLong("id");
    const ZiruStarters& starters = z27Starters();
    auto listed = starters.bySerialId.find({serial, id});
    if (listed != starters.bySerialId.end()) p = z27Clone(listed->second);
    if (!p && serial == 0) p = z27RentalCard(id);
    if (!p) {
      ObjP w = z28RecordFor(id, serial, false);
      if (w && w->optJSONObject("gameplayer")) p = z27Copy(w->getJSONObject("gameplayer"));
    }
    if (!p && serial != 0) p = z27RentalCard(id);
    auto any = starters.byId.find(id);
    if (!p && any != starters.byId.end()) p = z27Clone(any->second);
    auto upgraded = starters.byUpgradedId.find(id);
    if (!p && upgraded != starters.byUpgradedId.end()) p = z27Clone(upgraded->second);
    if (!p) {
      logRequest("Z27_PLAYER_UNKNOWN", "[Z27_PLAYER_UNKNOWN] no card data for id=" + str(id) + " serial=" + str(serial));
      return nullptr;
    }
    p->put("gameplayer_id", z27Copy(gid));
  }
  int64_t pid = gid->getLong("id");
  int team = p->optInt("initial_base_team_id", 0);
  int64_t upPid = z28ShownCardId(pid, team, gid->optLong("serial", 0));
  if (upPid != pid) {
    pid = upPid;
    gid->put("id", pid);
    if (p->optJSONObject("gameplayer_id")) p->getJSONObject("gameplayer_id")->put("id", pid);
  }

  // Learned skills first, so the card identity below keeps them on top of the card's own skills.
  std::vector<int32_t> al;
  if (fromSave) {
    std::string s = sp("almanac_state").getString("ziru_skills_" + str(serial), "");
    if (!s.empty()) {
      for (auto& part : split(s, ',')) {
        try {
          int v = ziruRepairLearnedSkill(parseInt(part));
          if (isZiruSkillId(v) && !vcontains(al, v) && al.size() < 5) al.push_back(v);
        } catch (...) {}
      }
    }
  }
  if (al.empty()) {
    ArrP addSkills = p->optJSONArray("additional_skill_id");
    if (addSkills) {
      for (int i = 0; i < addSkills->length(); i++) {
        int v = ziruRepairLearnedSkill(addSkills->optInt(i, -1));
        if (isZiruSkillId(v) && !vcontains(al, v) && al.size() < 5) al.push_back(v);
      }
    }
  }
  ArrP ja = newArr();
  for (int v : al) ja->put(v);
  p->put("additional_skill_id", ja);
  ArrP params = p->optJSONArray("parameters");
  if (params && params->length() >= 70) {
    for (int v : al) {
      if (isZiruSkillId(v)) {
        int pIdx = kZiruSkillParameters[v];
        if (isZiruSkillParameter(pIdx) && pIdx < params->length()) params->put(pIdx, 1);
      }
    }
  }
  // Same normalisation as the login roster, so the match uses what the card shows.
  z27NormalizeOwnedCard(p);
  return p;
}

// The captured start-game template's entry for this card, under the requested
// gameplayer_id. Used only when z27Player knows nothing about the card.
ObjP z27TemplateSquadPlayer(const ObjP& squad, const ObjP& gid) {
  ArrP list = squad ? squad->optJSONArray("player_list") : nullptr;
  for (int i = 0; list && i < list->length(); i++) {
    ObjP entry = list->optJSONObject(i);
    ObjP eid = entry ? entry->optJSONObject("gameplayer_id") : nullptr;
    if (eid && eid->optLong("id", 0) == gid->optLong("id", -1) && entry->optJSONArray("parameters")) {
      ObjP copy = z27Copy(entry);
      copy->put("gameplayer_id", z27Copy(gid));
      return copy;
    }
  }
  return nullptr;
}

ObjP z27Home(const ObjP& active) {
  ObjP home = nn(loadZiruJsonAssetObject("responses/ZIRU_START_GAME.json"))->getJSONObject("squad_data_home");
  ObjP sq = z27Squad(active->getInt("squad_id"));
  ArrP pl = sq->getJSONArray("player_list");
  ArrP out = newArr();
  for (int i = 0; i < pl->length(); i++) {
    ObjP gid = pl->getJSONObject(i)->getJSONObject("gameplayer_id");
    ObjP p = z27Player(gid);
    if (!p) p = z27TemplateSquadPlayer(home, gid);
    if (!p) continue;  // logged by z27Player; the match keeps this player's own card values
    int squadNum = pl->getJSONObject(i)->optInt("uniform_number", 0);
    if (squadNum <= 0) squadNum = p->optInt("uniform_number", i + 1);
    ObjP pObj = newObj()->put("gameplayer_id", z27Copy(gid))
                       ->put("parameters", Array::parse(p->getJSONArray("parameters")->toString()))
                       ->put("averaged_overall", 0)
                       ->put("uniform_number", squadNum);
    ArrP ib = p->optJSONArray("initial_booster_id_list");
    if (ib) pObj->put("initial_booster_id_list", Array::parse(ib->toString()));
    // Learned skills come with the card (z27Player reads them for the card saved
    // under its serial only); a card known only to the template has none.
    ArrP sk = p->optJSONArray("additional_skill_id");
    if (sk) pObj->put("additional_skill_id", Array::parse(sk->toString()));
    ArrP ps = p->optJSONArray("play_style");
    if (ps) pObj->put("play_style", Array::parse(ps->toString()));
    ArrP params = pObj->getJSONArray("parameters");
    if (sk && params && params->length() >= 142) {
      for (int sIdx = 0; sIdx < sk->length(); ++sIdx) {
        int v = sk->optInt(sIdx);
        if (isZiruSkillId(v)) {
          int pIdx = kZiruSkillParameters[v];
          if (isZiruSkillParameter(pIdx) && pIdx < params->length()) params->put(pIdx, 1);
        }
      }
    }
    if (params && params->length() >= 70) {
      ArrP baseSkills = newArr();
      std::set<int> addSkillSet;
      if (sk) {
        for (int si = 0; si < sk->length(); ++si) addSkillSet.insert(sk->optInt(si, -1));
      }
      for (int s = 0; s < 74; ++s) {
        int pIdx = kZiruSkillParameters[s];
        if (isZiruSkillParameter(pIdx) && pIdx < params->length() && params->optInt(pIdx, 0) == 1) {
          if (addSkillSet.find(s) == addSkillSet.end()) {
            baseSkills->put(s);
          }
        }
      }
      pObj->put("skillcard_id_list", baseSkills);
    }
    out->put(pObj);
  }
  ObjP gp = z27Copy(sq->getJSONObject("squad_gameplan"));
  ObjP sdWrap = newObj()->put("squad_gameplan", gp);
  z27ActivateCoachLink(sdWrap);
  home->put("player_list", out)->put("squad_gameplan", gp)->put("team_id", active->getJSONObject("home")->getInt("base_team_id"));
  home->put("base_team", newArr()->put(active->getJSONObject("home")));
  ArrP co = sq->optJSONArray("coach");
  ObjP coach;
  if (!co || co->length() == 0) {
    coach = resolveZiruCoachFromLoginV73(17608292273375LL);
    if (!coach) coach = resolveZiruCoachFromLoginV73(8388824LL);
  } else {
    ObjP ref = co->getJSONObject(0);
    int64_t cid = ref->getJSONObject("coach_id")->getLong("id");
    coach = resolveZiruCoachFromLoginV73(cid);
    if (!coach) coach = resolveZiruCoachFromLoginV73(17608292273375LL);
    if (coach && ref->optJSONObject("coach_id")) {
      coach->put("coach_id", z27Copy(ref->getJSONObject("coach_id")));
    }
  }
  if (!coach) throw IllegalState("Unknown HOME coach");
  home->put("coach", coach);
  return home;
}

ArrP z27SavedKits(int32_t sq) {
  JStr m = loadMeta("z27_kits_" + str(sq));
  if (!m) return nullptr;
  return Array::parse(*m);
}

ArrP z27AwayKits(const ObjP& active, const ObjP& homeUniform) {
  ObjP ht = z27Team(active->getJSONObject("home")->getInt("base_team_id"));
  ObjP at = z27Team(active->getJSONObject("away")->getInt("base_team_id"));
  int first = homeUniform->getJSONArray("uniform_list")->getInt(0);
  ArrP rgb;
  ArrP hk = ht->getJSONArray("kits"), ak = at->getJSONArray("kits");
  for (int i = 0; i < hk->length(); i++)
    if (hk->getJSONObject(i)->getInt("id") == first) rgb = hk->getJSONObject(i)->getJSONArray("shirt_rgb");
  int best = -1, bi = 0;
  for (int i = 0; i < ak->length(); i++) {
    ArrP c = ak->getJSONObject(i)->getJSONArray("shirt_rgb");
    int d = 0;
    for (int k = 0; k < 3; k++) {
      int x = c->getInt(k) - nn(rgb)->getInt(k);
      d = x * x + d;
    }
    if (d > best) { best = d; bi = i; }
  }
  ArrP out = newArr()->put(ak->getJSONObject(bi)->getInt("id"));
  for (int i = 0; i < ak->length(); i++) if (i != bi) out->put(ak->getJSONObject(i)->getInt("id"));
  return out;
}

ObjP z27ContrastAwayUniform(int32_t homeTeamId, int32_t awayTeamId, const ObjP& homeUniform) {
  ObjP ht = z27Team(homeTeamId);
  ObjP at = z27Team(awayTeamId);
  if (!at) return z27Uniform(z27Team(awayTeamId), nullptr);
  ArrP ak = at->optJSONArray("kits");
  if (!ak || ak->length() == 0 || !ht || !homeUniform) return z27Uniform(at, nullptr);
  ArrP huList = homeUniform->optJSONArray("uniform_list");
  int first = (huList && huList->length() > 0) ? huList->optInt(0, 0) : 0;
  ArrP rgb = nullptr;
  ArrP hk = ht->optJSONArray("kits");
  if (hk) {
    for (int i = 0; i < hk->length(); i++) {
      ObjP k = hk->optJSONObject(i);
      if (k && k->optInt("id", 0) == first) { rgb = k->optJSONArray("shirt_rgb"); break; }
    }
    if (!rgb && hk->length() > 0 && hk->optJSONObject(0)) rgb = hk->optJSONObject(0)->optJSONArray("shirt_rgb");
  }
  if (!rgb) return z27Uniform(at, nullptr);
  int best = -1, bi = 0;
  for (int i = 0; i < ak->length(); i++) {
    ObjP k = ak->optJSONObject(i);
    ArrP c = k ? k->optJSONArray("shirt_rgb") : nullptr;
    if (!c || c->length() < 3) continue;
    int d = 0;
    for (int idx = 0; idx < 3; idx++) {
      int x = c->optInt(idx, 0) - rgb->optInt(idx, 0);
      d += x * x;
    }
    if (d > best) { best = d; bi = i; }
  }
  ArrP out = newArr();
  if (ak->optJSONObject(bi)) out->put(ak->optJSONObject(bi)->optInt("id", 0));
  for (int i = 0; i < ak->length(); i++) {
    if (i != bi && ak->optJSONObject(i)) out->put(ak->optJSONObject(i)->optInt("id", 0));
  }
  return newObj()->put("base_team_id", awayTeamId)->put("uniform_list", out);
}

ObjP z27AwaySquad(const ObjP& active) {
  ObjP opp = z27Opponent(active);
  ObjP sd = opp ? opp->optJSONObject("squad_data") : nullptr;
  ArrP players = opp ? opp->optJSONArray("player_list") : nullptr;
  ObjP coach = opp ? opp->optJSONObject("coach_data") : nullptr;
  if (!coach && sd && sd->optJSONArray("coach") && sd->getJSONArray("coach")->length() > 0) {
    int64_t cid = sd->getJSONArray("coach")->getJSONObject(0)->getJSONObject("coach_id")->getLong("id");
    coach = resolveZiruCoachFromLoginV73(cid);
  }
  if (!coach) coach = resolveZiruCoachFromLoginV73(17608560707469LL);
  if (!coach) coach = resolveZiruCoachFromLoginV73(17608292273375LL);

  ObjP awaySquad = newObj();
  int awayTeamId = active->getJSONObject("away")->getInt("base_team_id");
  awaySquad->put("team_id", awayTeamId);
  awaySquad->put("base_team", newArr()->put(active->getJSONObject("away")));
  awaySquad->put("coach", z27Copy(coach));

  ArrP playerList = newArr();
  if (players) {
    for (int i = 0; i < players->length(); i++) {
      ObjP p = players->getJSONObject(i);
      ObjP gid = p->getJSONObject("gameplayer_id");
      ObjP pObj = newObj()->put("gameplayer_id", z27Copy(gid))
                         ->put("parameters", Array::parse(p->getJSONArray("parameters")->toString()))
                         ->put("averaged_overall", 0)
                         ->put("uniform_number", p->optInt("uniform_number", i + 1));
      ArrP ps = p->optJSONArray("play_style");
      if (ps) pObj->put("play_style", Array::parse(ps->toString()));
      ArrP sk = p->optJSONArray("additional_skill_id");
      if (sk) pObj->put("additional_skill_id", Array::parse(sk->toString()));
      ArrP params = pObj->getJSONArray("parameters");
      if (params && params->length() >= 70) {
        ArrP baseSkills = newArr();
        std::set<int> addSkillSet;
        if (sk) {
          for (int si = 0; si < sk->length(); ++si) addSkillSet.insert(sk->optInt(si, -1));
        }
        for (int s = 0; s < 74; ++s) {
          int pIdx = kZiruSkillParameters[s];
          if (isZiruSkillParameter(pIdx) && pIdx < params->length() && params->optInt(pIdx, 0) == 1) {
            if (addSkillSet.find(s) == addSkillSet.end()) {
              baseSkills->put(s);
            }
          }
        }
        pObj->put("skillcard_id_list", baseSkills);
      }
      playerList->put(pObj);
    }
  }
  awaySquad->put("player_list", playerList);

  ObjP gp = sd ? z27Copy(sd->getJSONObject("squad_gameplan")) : newObj();
  ObjP sdWrap = newObj()->put("squad_gameplan", gp);
  z27ActivateCoachLink(sdWrap);
  awaySquad->put("squad_gameplan", gp);

  return awaySquad;
}

Response z27Start() {
  try {
    ObjP req = z27Request();
    ObjP act = z27Active();
    int64_t gid = req ? req->optLong("game_id", act->getLong("game_id")) : act->getLong("game_id");
    if (gid <= 0) gid = act->getLong("game_id");
    act->put("game_id", (long long)gid);
    ObjP completed = z27State()->optJSONObject("completed");
    if (completed) completed->remove(str(gid));
    int mainId = z27Main();
    ObjP current = z27Copy(act);
    current->put("squad_id", mainId)->put("home", z27Base(mainId));
    ObjP home = z27Home(current);
    ObjP hu = z27Uniform(current->getJSONObject("home"), z27SavedKits(mainId));
    bool reuse = act->optBoolean("started", false) && act->getInt("squad_id") == mainId &&
        act->optJSONObject("home_squad") && act->optJSONObject("home_uniform") &&
        act->getJSONObject("home_squad")->toString() == home->toString() &&
        act->getJSONObject("home_uniform")->toString() == hu->toString() &&
        act->getJSONObject("home")->toString() == current->getJSONObject("home")->toString();
    if (!reuse) {
      act->put("squad_id", mainId)->put("home", current->getJSONObject("home"));
      act->put("home_squad", home);
      act->put("home_uniform", hu)->put("away_uniform", z27Uniform(act->getJSONObject("away"), z27AwayKits(act, hu)))->put("started", true);
      z27PutMeta("z27_active_match", act->toString());
    }
    gZiruHomeTeamId = current->getJSONObject("home")->optInt("base_team_id", 0);
    gZiruAwayTeamId = act->getJSONObject("away")->optInt("base_team_id", 0);
    ObjP out = nn(loadZiruJsonAssetObject("responses/ZIRU_START_GAME.json"));
    out->put("rqid", (long long)req->optLong("rqid", 1))
        ->put("game_id", (long long)gid)
        ->put("squad_data_home", z27Copy(act->getJSONObject("home_squad")))
        ->put("uniform_list_home", z27Copy(act->getJSONObject("home_uniform")))
        ->put("uniform_list_away", z27Copy(act->getJSONObject("away_uniform")));
    ObjP awaySquad = z27AwaySquad(act);
    out->put("squad_data_away", awaySquad);
    ObjP ci = out->optJSONObject("commentary_info");
    if (ci) ci->put("pre_division", act->getInt("division"));
    logRequest("ZIRU_MATCH_START", "game=" + str(gid) + " squad=" + str(mainId) + " coach=" +
        home->getJSONObject("coach")->getJSONObject("coach_id")->toString() + " reused=" + str(reuse) + " division=" + str(act->getInt("division")) + " cpu=" + act->getString("cpu_level"));
    return serveObjectAsEncryptedMsgPack(out);
  } catch (const std::exception& e) {
    return z27Error("CMD_START_GAME", e);
  }
}

int32_t z27Score(const ObjP& row) {
  if (!row) throw IllegalArg("Missing score row");
  int sum = 0;
  bool any = false;
  for (const char* k : {"score_1st", "score_2nd", "score_ex1st", "score_ex2nd"}) {
    if (row->has(k)) {
      int v = row->getInt(k);
      if (v < 0) throw IllegalArg("Negative score");
      sum = v + sum;
      any = true;
    }
  }
  if (any) return sum;
  throw IllegalArg("No score fields");
}

ObjP z27History(const ObjP& act, const ObjP& req, int32_t a, int32_t b, const std::string& res) {
  ObjP h = z27Copy(nn(loadZiruJsonAssetObject("responses/ZIRU_GET_GAMERESULT_LIST.json"))->getJSONArray("game_result_list")->getJSONObject(0));
  ObjP home = act->getJSONObject("home"), away = act->getJSONObject("away");
  std::string ge = !req->optJSONObject("match_result") ? std::string("NORMAL") : req->getJSONObject("match_result")->optString("abnormalend_reason", "NORMAL");
  h->put("game_id", (long long)act->getLong("game_id"))
      ->put("date", (long long)(zc::currentTimeMillis() / 1000))
      ->put("is_com_match", "YES")
      ->put("gameend_reason", ge)
      ->put("team", newArr()->put(home->getInt("base_team_id"))->put(away->getInt("base_team_id")))
      ->put("emblem_team", newArr()->put(home->getInt("base_team_id"))->put(away->getInt("base_team_id")))
      ->put("team_name", newArr()->put(home->getString("team_name"))->put(away->getString("team_name")))
      ->put("team_name_short", newArr()->put(home->getString("team_name_short"))->put(away->getString("team_name_short")))
      ->put("score", Array::parse(req->getJSONArray("score")->toString()))
      ->put("division", act->getInt("division"))
      ->put("opponent_profile_id", act->getJSONObject("profile")->getInt("profile_id"))
      ->put("game_res", res)
      ->put("home_score", a)
      ->put("away_score", b);
  ArrP pl = h->getJSONArray("player");
  pl->getJSONObject(0)->put("is_com", "NO");
  pl->getJSONObject(1)->put("is_com", "YES")->put("cpu_level", act->getString("cpu_level"))->put("user_info", z27Opponent(act)->getJSONArray("user_info")->getJSONObject(0));
  return h;
}

Response z27Result() {
  try {
    ObjP req = z27Request();
    ObjP act = z27Active();
    ObjP st = z27State();
    ObjP mr = req->optJSONObject("match_result");
    int64_t gid = !mr ? (req ? req->optLong("game_id", 0) : 0) : mr->optLong("game_id", req ? req->optLong("game_id", 0) : 0);
    if (gid <= 0) gid = act->getLong("game_id");
    act->put("game_id", (long long)gid);
    act->put("started", true);
    ObjP cached = z27Meta("z27_result_" + str(gid));
    if (cached) return serveObjectAsEncryptedMsgPack(cached->getJSONObject("response")->put("rqid", (long long)(req ? req->optLong("rqid", 1) : 1)));
    ObjP completed = st->getJSONObject("completed");
    std::string key = str(gid);
    ObjP resp;
    ObjP m2 = z27Meta("z27_result_" + key);
    if (m2) {
      resp = z27Copy(m2->getJSONObject("response"));
    } else {
      ArrP sc = req->optJSONArray("score");
      if (!sc || sc->length() != 2) throw IllegalArg("Two actual score rows required; no inferred result");
      int a = z27Score(sc->getJSONObject(0));
      int b = z27Score(sc->getJSONObject(1));
      std::string res = a > b ? "WIN" : (a < b ? "LOSE" : "DRAW");
      bool giveup = req->optString("intentional_give_up") == "YES" && mr && mr->optString("abnormalend_reason") == "NORMAL_GIVEUP";
      if (giveup && a <= b) res = "LOSE";
      int div = st->getInt("division");
      int pt = st->getInt("point");
      int gain;
      if (res == "WIN") gain = 3;
      else if (res == "DRAW") gain = 1;
      else gain = 0;
      int np = pt + gain;
      int nm = st->getInt("matches") + 1;
      st->put("point", np)->put("matches", nm);
      std::string k = res == "WIN" ? "win" : res == "DRAW" ? "draw" : "lose";
      st->put(k, st->getInt(k) + 1);
      ObjP hist = z27History(act, req, a, b, res);
      st->put("sequence", st->getInt("sequence") + 1);
      // The menu advertises this point target. Do not hold an already qualified
      // player in the same division until all ten fixtures have been played.
      bool roundEnded = nm >= 10 || (div > 1 && np >= z27Promotion(div));
      if (roundEnded) {
        int nd;
        if (np < z27Promotion(div) || div <= 1) nd = (np >= std::max(0, z27Promotion(div) + (-5)) || div >= 10) ? div : div + 1;
        else nd = div - 1;
        st->put("division", nd)->put("best", std::min(st->getInt("best"), nd))->put("point", 0)->put("matches", 0)->put("win", 0)->put("draw", 0)->put("lose", 0);
      }
      resp = nn(loadZiruJsonAssetObject("responses/ZIRU_GET_VSCOM_GAME_RESULT_TEMPLATE.json"));
      ObjP ge = resp->getJSONObject("game_end_result");
      ObjP mres = ge->getJSONObject("match_result");
      mres->getJSONObject("player_data")->put("team_id", act->getJSONObject("home")->getInt("base_team_id"));
      ObjP an = ge->optJSONObject("analyst_get_game_result_info");
      if (an) an->put("advice_info_match_list", newArr());
      mres->put("game_res", res)
          ->put("gameend_reason", giveup ? "NORMAL_GIVEUP" : "NORMAL")
          ->put("abnormalend_reason", giveup ? "NORMAL_GIVEUP" : "NORMAL")
          ->put("score_data", newObj()->put("home_score", a)->put("away_score", b));
      ObjP ex = ge->optJSONObject("extra_match_result");
      ObjP lm;
      if (ex && (lm = ex->optJSONObject("league_match_result"))) {
        z27InfoState(lm, st);
        ObjP li = lm->optJSONObject("league_info");
        if (!li) li = lm;
        li->put("is_promoted", st->getInt("division") < div ? "YES" : "NO")
            ->put("is_relegated", st->getInt("division") > div ? "YES" : "NO")
            ->put("is_staying", (!roundEnded || st->getInt("division") != div) ? "NO" : "YES");
      }
      completed->put(key, true);
      ziruDb->saveMatchState(st->toString(), "z27_result_" + key, newObj()->put("history", hist)->put("response", z27Copy(resp))->toString());
      if (!z27Meta("z27_result_" + key)) throw IllegalState("Result transaction missing");
      logRequest("ZIRU_RESULT_COMMIT", "game=" + str(gid) + " " + str(a) + "-" + str(b) + " " + res +
          " division=" + str(div) + "->" + str(st->getInt("division")) + " points=" + str(np) + " round=" + str(nm));
    }
    resp->put("rqid", (long long)req->optLong("rqid", 1));
    return serveObjectAsEncryptedMsgPack(resp);
  } catch (const std::exception& e) {
    return z27Error("CMD_GET_VSCOM_GAME_RESULT", e);
  }
}

Response z27HistoryResponse() {
  try {
    ObjP req = z27Request();
    ObjP tpl = loadZiruJsonAssetObject("responses/ZIRU_GET_GAMERESULT_LIST.json");
    ObjP completed = z27State()->getJSONObject("completed");
    std::vector<int64_t> ids;
    for (auto& k : completed->keys()) ids.push_back(parseLong(k));
    std::sort(ids.begin(), ids.end(), std::greater<int64_t>());
    ArrP list = newArr();
    for (int64_t id : ids) list->put(nn(z27Meta("z27_result_" + str(id)))->getJSONObject("history"));
    ArrP lh = Array::parse(ziruDb->loadLeagueHistory());
    for (int i = 0; i < lh->length(); i++)
      if (!completed->has(str(lh->getJSONObject(i)->optLong("game_id")))) list->put(lh->getJSONObject(i));
    nn(tpl)->put("game_result_list", list)->put("rqid", (long long)req->optLong("rqid", 1));
    return serveObjectAsEncryptedMsgPack(tpl);
  } catch (const std::exception& e) {
    return z27Error("CMD_GET_GAMERESULT_LIST", e);
  }
}

std::optional<Response> z27Route(const std::string& uri) {
  std::string up = toUpperUS(replaceAll(uri, "_", ""));
  if (contains(up, "CMDSETMYCLUBSQUADINFO")) return z27SquadAck();
  if (contains(up, "CMDGETUSEABLETEAMLIST")) {
    ObjP jo = loadZiruJsonAssetObject("responses/get_useable_team_list.json");
    ObjP req = z27Request();
    ArrP ids = newArr();
    for (auto& k : z27Teams()->keys()) ids->put(parseInt(k));
    nn(jo)->put("useable_team_list", ids)->put("rqid", (long long)req->optLong("rqid", 1));
    return serveObjectAsEncryptedMsgPack(jo);
  }
  if (contains(up, "CMDGETMYCLUBENTRYINFO") || contains(up, "CMDGETENTRYINFO")) {
    ObjP entry = z27Entry();
    entry->put("current_gameplayer_warehouse_list_max", 100000);
    entry->put("warehouse_player_max", 100000);
    entry->put("current_gameplayer_list_max", 100000);
    entry->put("gameplayer_list_max", 100000);
    entry->put("unlock_gameplayer_frame", 100000);
    entry->put("unlock_gameplayer_warehouse_frame", 100000);
    ObjP put = z27Ack("CMD_GET_MYCLUB_ENTRY_INFO", z27Request())->put("entry_info", entry);
    ZiruTx tx(*ziruDb);
    z27Restore(put);
    tx.commit();
    return serveObjectAsEncryptedMsgPack(put);
  }
  for (const char* c : {"CMD_UPDATE_MYCLUB_BASE_TEAM", "CMD_SET_MYCLUB_MAIN_SQUAD", "CMD_SET_MYCLUB_BRING_UNIFORM", "CMD_SET_MYCLUB_ENTRY_INFO"}) {
    if (contains(up, replaceAll(c, "_", ""))) return z27TeamChange(c);
  }
  return std::nullopt;
}
