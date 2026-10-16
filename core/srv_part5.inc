// ---- part 5 (OfflineServer.java ~2805-3322) ----

ObjP refreshZiruGameplayerFromCanonical(const ObjP& jo) {
  if (!jo) return jo;
  try {
    ObjP gid = jo->optJSONObject("gameplayer_id");
    if (gid) {
      int64_t id = gid->optLong("id", 0);
      int64_t serial = gid->optLong("serial", 0);
      if (id > 0) {
        ObjP cur = jo;
        JStr lp;
        if (serial > 0 && (lp = loadPlayer(serial)) && !lp->empty()) {
          cur = Object::parse(*lp);
          ObjP gp = cur->optJSONObject("gameplayer");
          if (gp) cur = gp;
          ObjP g2 = cur->optJSONObject("gameplayer_id");
          if (!g2 || g2->optLong("id") != id) cur = jo;
        }
        ObjP w = getZiruPlayerWrapper(Value((long long)id), false);
        ObjP base;
        if (w && (base = w->optJSONObject("gameplayer"))) {
          ArrP ib;
          if (!cur->optJSONArray("initial_booster_id_list") && (ib = base->optJSONArray("initial_booster_id_list"))) cur->put("initial_booster_id_list", ib);
          ObjP bl = base->optJSONObject("level_info");
          int lim = bl ? bl->optInt("level_limit", 1) : 1;
          if (lim == 1 && (((int32_t)id) & 16777215) == 134665) {
            lim = 33;
            logRequest("ZIRU_V16_LEVEL", "[ZIRU_V16_ADRIANO_FALLBACK] base_id=134665 level_limit=33");
          }
          ObjP li = cur->optJSONObject("level_info");
          if (!li) { li = newObj(); cur->put("level_info", li); }
          li->put("level_limit", lim);
          li->put("level", lim);
          li->put("exp_total", 0);
          li->put("exp_current_level", 0);
          li->put("exp_to_next_level", 0);
          int cp = (lim - 1) * 2;
          if (serial > 0) {
            JStr pr = loadProgression(serial);
            if (!pr || pr->empty()) {
              ArrP a = cur->optJSONArray("parameters");
              ArrP b = base->optJSONArray("parameters");
              if (a && b && a->toString() != b->toString()) cp = 0;
            } else {
              cp -= Object::parse(*pr)->optInt("_spent_total", 0);
              if (cp < 0) cp = 0;
            }
          }
          li->put("customize_point", cp);
        }
        return ziruSyncActiveBooster(cur);
      }
    }
    return jo;
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V14_PLAYER_NORMALIZE_ERR] ") + e.what());
    return jo;
  }
}

ObjP resolveZiruCoachFromLoginV73(int64_t id) {
  try {
    ObjP login = loadZiruJsonAssetObject("responses/CmdLogin.json");
    ObjP ei;
    ArrP cl;
    if (!login || !(ei = login->optJSONObject("entry_info")) || !(cl = ei->optJSONArray("coach_list"))) return nullptr;
    for (int i = 0; i < cl->length(); i++) {
      ObjP c = cl->optJSONObject(i);
      ObjP cid;
      if (c && (cid = c->optJSONObject("coach_id")) && cid->optLong("id", 0) == id) return c;
    }
    return nullptr;
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V73_ML_COACH_RESOLVE_ERR] ") + e.what());
    return nullptr;
  }
}

std::string resolveZiruSkillCategory() {
  if (ziruSkillCategory) {
    std::string s = *ziruSkillCategory;
    size_t b = 0, e = s.size();
    while (b < e && (unsigned char)s[b] <= ' ') b++;
    while (e > b && (unsigned char)s[e - 1] <= ' ') e--;
    std::string t = s.substr(b, e - b);
    if (!t.empty()) return toUpperUS(t);
  }
  int i = ziruSkillCategoryCode;
  if (i >= 0) {
    if (i == 0) return "SHOOT";
    if (i == 1) return "PASS";
    if (i == 2) return "DRIBBLE";
    if (i == 3) return "DEFENCE";
    if (i == 4 || i == 5) return "OTHER";
  }
  int i2 = ziruSkillItemEffect;
  if (i2 <= 0) i2 = ziruSkillItemGrade;
  switch (i2) {
    case 1: return "SHOOT";
    case 2: return "PASS";
    case 3: return "DRIBBLE";
    case 4: return "DEFENCE";
    case 5: return "OTHER";
    default: return "UNKNOWN";
  }
}

void sanitizeZiruAdditionalSkills(const ObjP& jo) {
  try {
    ObjP ei = jo->optJSONObject("entry_info");
    ArrP list;
    if (!ei || !(list = ei->optJSONArray("gameplayer_list"))) return;
    for (int i = 0; i < list->length(); i++) {
      ObjP p = list->optJSONObject(i);
      ArrP sk;
      if (p && (sk = p->optJSONArray("additional_skill_id"))) {
        ArrP n = newArr();
        for (int k = 0; k < sk->length(); k++) {
          int v = sk->optInt(k);
          if (v != 6 && v != 32 && v != 70) n->put(v);
        }
        p->put("additional_skill_id", n);
      }
    }
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_SKILL_V21_SANITIZE_ERR] ") + e.what());
  }
}

void saveZiruLeagueHistory(int64_t gameId, int32_t a, int32_t b) {
  try {
    JStr m = loadMeta("ziru_league_history_json");
    ArrP hist = newArr();
    if (m && !m->empty()) hist = Array::parse(*m);
    JStr m2 = loadMeta("ziru_league_last_game_id");
    int64_t j2 = gameId;
    if (m2 && !m2->empty()) j2 = parseLong(*m2);
    ObjP item = buildZiruLeagueHistoryItem(j2, a, b);
    if (item) {
      JStr m3 = loadMeta("ziru_league_last_gameend_reason");
      if (m3 && !m3->empty()) item->put("gameend_reason", *m3);
      ziruDb->saveLeagueHistory(gameId, item->toString());
      ArrP out = newArr();
      out->put(item);
      int len = hist->length();
      for (int i = 0; i < len && i < 49; i++) {
        ObjP o = hist->optJSONObject(i);
        if (o) out->put(o);
      }
      ziruDb->saveMeta("ziru_league_history_json", out->toString());
    }
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V64_HISTORY_SAVE_ERR] ") + e.what());
  }
}

void saveZiruPlayerToDb(const ObjP& jo) {
  if (!jo) return;
  ObjP gid = jo->optJSONObject("gameplayer_id");
  if (gid) {
    int64_t s = gid->optLong("serial", 0);
    int64_t id = gid->optLong("id", 0);
    if (s > 0) ziruDb->savePlayer(s, id, jo->toString());
  }
}

void saveZiruSkillsToDb(int64_t serial, const std::vector<int32_t>& list) { z27SaveSkills(serial, list); }

Response serveLivedataAsset(const std::string& name) {
  std::string data;
  if (!assets.read(name, data)) return newFixedText(404, "LiveData asset not found");
  zs::logi("OFFLINE_SERVER", "[ALMANAC_V8] Served LiveData asset: " + name);
  return newFixed(200, "application/octet-stream", Bytes(data.begin(), data.end()));
}

Response serveObjectAsEncryptedMsgPack(const ObjP& jo) {
  // packValue(null) packs nil – Java does not throw here
  Response r = newFixed(200, "application/x-msgpack", zc::encodeResponse(jo ? Value(jo) : Value()));
  r.headers["Connection"] = "close";
  return r;
}

void sortZiruMlLeaderboardV85(const ArrP& a) {
  if (!a) return;
  int len = a->length();
  for (int i = 0; i < len - 1; i++) {
    int lim = (len - 1) - i;
    for (int k = 0; k < lim; k++) {
      ObjP x = a->optJSONObject(k), y = a->optJSONObject(k + 1);
      if (x && y && x->optInt("value") < y->optInt("value")) { a->put(k, y); a->put(k + 1, x); }
    }
  }
}

void sortZiruMyLeagueRankingV83(const ArrP& a) {
  if (!a) return;
  int len = a->length();
  for (int i = 0; i < len - 1; i++) {
    int lim = (len - 1) - i;
    for (int k = 0; k < lim; k++) {
      ObjP x = a->optJSONObject(k), y = a->optJSONObject(k + 1);
      if (!x || !y) continue;
      int p1 = x->optInt("winning_points"), p2 = y->optInt("winning_points");
      bool sw = false;
      if (p1 < p2) sw = true;
      else if (p1 == p2) {
        int d1 = x->optInt("diff"), d2 = y->optInt("diff");
        if (d1 < d2) sw = true;
        else if (d1 == d2 && x->optInt("goals") < y->optInt("goals")) sw = true;
      }
      if (sw) { a->put(k, y); a->put(k + 1, x); }
    }
  }
  for (int i = 0; i < len; i++) {
    ObjP o = a->optJSONObject(i);
    if (o) o->put("rank", i + 1);
  }
}

void traceZiruUserCompeV87(const std::string& uri) {
  if (!contains(uri, "USER_COMPE")) return;
  zs::logi("ZIRU_WC", "[ZIRU_V87_USER_COMPE_URI] " + uri);
  ObjP req;
  if (ziruLastRequestBody && (req = decodeZiruRequestJson(ziruLastRequestBody)))
    zs::logi("ZIRU_WC", "[ZIRU_V87_USER_COMPE_REQ] msgid=" + req->optString("msgid", "") + " body=" + req->toString());
}

ObjP z27OwnedPlayer(int64_t serial) {
  JStr lp = loadPlayer(serial);
  if (lp) {
    ObjP jo = Object::parse(*lp);
    ObjP gp = jo->optJSONObject("gameplayer");
    return gp ? gp : jo;
  }
  std::string s = sp("almanac_state").getString("recruited_players", "[]");
  if (s.empty()) s = "[]";
  ArrP arr = Array::parse(s);
  bool second = false;
  while (true) {
    for (int i = 0; i < arr->length(); i++) {
      ObjP o = arr->optJSONObject(i);
      if (!o) continue;
      ObjP gp = o->optJSONObject("gameplayer");
      if (gp) o = gp;
      ObjP gid = o->optJSONObject("gameplayer_id");
      if (gid && gid->optLong("serial") == serial) return o;
    }
    if (second) return nullptr;
    arr = z27Entry()->getJSONArray("gameplayer_list");
    second = true;
  }
}

void z27RepairLoginRoster(const ObjP& jo) {
  try {
    ObjP ei = jo->optJSONObject("entry_info");
    ArrP list;
    if (!ei || !(list = ei->optJSONArray("gameplayer_list"))) return;
    ArrP out = newArr();
    std::set<int64_t> seen;
    for (int i = 0; i < list->length(); i++) {
      ObjP p = list->optJSONObject(i);
      ObjP gid;
      if (p && (gid = p->optJSONObject("gameplayer_id"))) {
        int64_t s = gid->optLong("serial");
        if (!isZiruPlayerSold(s) && seen.insert(s).second) out->put(refreshZiruGameplayerFromCanonical(p));
      }
    }
    ei->put("gameplayer_list", out);
    applyZiruSkillStateToLogin(jo);
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("Login roster repair failed ") + e.what());
  }
}

void z28PreserveCareerTraining(const ObjP& jo, const ArrP& params) {
  ziruSyncActiveBooster(jo);
  ArrP cur = jo->optJSONArray("parameters");
  if (cur) {
    if (cur->length() < 145 || params->length() < 145) return;
    for (int i = 0; i < 3; i++) params->put(142 + i, cur->optInt(142 + i));
  }
}

int32_t z28RandomBooster() {
  ArrP a = nn(loadZiruJsonAssetObject("responses/ZIRU_RANDOM_BOOSTERS.json"))->getJSONArray("ability_ids");
  zc::JavaRandom r;
  return a->getInt(r.nextInt(a->length()));
}

void ziruAttachResetCounts(const ObjP& jo, int64_t agent) {
  try {
    ArrP list = nn(loadZiruJsonAssetObject("responses/CmdGetMyclubAgentlist.json"))->optJSONArray("agent_list");
    if (!list) return;
    for (int i = 0; i < list->length(); i++) {
      ObjP a = list->optJSONObject(i);
      if (a && a->optLong("agent_id") == agent) {
        ArrP b = a->optJSONArray("base_player_count");
        if (b) {
          ArrP c = Array::parse(b->toString());
          jo->put("remaining_player_count", c);
          jo->put("base_player_count", c);
          jo->put("can_reset", "YES");
        }
        return;
      }
    }
  } catch (const std::exception&) {
  }
}

int32_t ziruCountFromSpent(int32_t spent) {
  int c = 0, step = 1, rem = spent;
  while (rem >= step) {
    rem -= step;
    c++;
    if (c % 4 == 0) step++;
  }
  return c;
}

std::string ziruLeagueCpuLevel(int i) {
  if (i == 8) return "EASY";
  if (i == 7) return "NORMAL";
  if (i == 6) return "HARD";
  if (i == 5) return "VERYHARD";
  return i < 3 ? "LEGEND" : "SUPERSTAR";
}

int32_t ziruLeagueLoadInt(const std::string& k, int32_t def) {
  JStr m = loadMeta(k);
  if (m) {
    try { return parseInt(*m); } catch (const std::exception&) {}
  }
  return def;
}

int32_t ziruLeaguePromotionThreshold(int i) {
  if (i == 8) return 15;
  if (i == 7) return 16;
  if (i != 6) return i != 5 ? 19 : 18;
  return 17;
}

int32_t ziruLeagueRoundLimit(int i) { return (i == 10 || i == 9) ? 8 : 10; }

void ziruLeagueSaveInt(const std::string& k, int32_t v) { ziruDb->saveMeta(k, str(v)); }
