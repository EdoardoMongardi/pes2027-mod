// ---- part 5 (OfflineServer.java ~2805-3322) ----

void z27ApplyAppearanceAndClub(const ObjP& cur) {
  if (!cur) return;
  ObjP gid = cur->optJSONObject("gameplayer_id");
  if (!gid) return;
  int64_t pid = gid->optLong("id", 0);
  int32_t base_id = (int32_t)(pid & 0xFFFFFF);

  // 1. Fix club kit if initial_base_team_id == 0
  if (cur->optInt("initial_base_team_id", 0) == 0) {
    auto it = kKnownClubTeams.find(base_id);
    if (it != kKnownClubTeams.end()) {
      cur->put("initial_base_team_id", it->second);
    }
  }

  // 2. Fix appearance parameters [49:65] if all zeroes or pitch black/albino (skin 6 / 0 bug)
  auto fixAppearance = [&](const ArrP& params) {
    if (!params || params->length() < 65) return;
    bool allZero = true;
    for (int i = 49; i <= 60; i++) {
      if (params->optInt(i, 0) != 0) {
        allZero = false;
        break;
      }
    }
    int skin = params->optInt(49, 0);
    bool pitchBlackOrAlbino = (skin == 6 || skin == 0);
    if (allZero || pitchBlackOrAlbino) {
      auto it = kAppearancePresets.find(base_id);
      if (it != kAppearancePresets.end()) {
        for (int i = 0; i < 16; i++) {
          params->put(49 + i, (int)it->second.v[i]);
        }
      } else {
        // Natural default skin tone and appearance so player is never chalk white or pitch black
        static const int32_t kDefAppearance[16] = {3, 1, 1, 1, 4, 0, 4, 18, 4, 8, 142, 0, 14, 0, 1, 1};
        for (int i = 0; i < 16; i++) {
          params->put(49 + i, kDefAppearance[i]);
        }
      }
    }
  };

  fixAppearance(cur->optJSONArray("parameters"));
  fixAppearance(cur->optJSONArray("original_parameters"));
}

void z27ApplyProgressionPoints(const ArrP& params, const ObjP& prog, const ArrP& baseParams) {
  if (!params || !prog || params->length() < 145) return;
  const ArrP& src = (baseParams && baseParams->length() >= 145) ? baseParams : params;

  int strengthCount = ziruCountFromSpent(prog->optInt("STRENGTH", 0));
  int shootCount    = ziruCountFromSpent(prog->optInt("SHOOT", 0));
  int passCount     = ziruCountFromSpent(prog->optInt("PASS", 0));
  int dribbleCount  = ziruCountFromSpent(prog->optInt("DRIBLE", 0) + prog->optInt("DRIBBLE", 0));
  int quickCount    = ziruCountFromSpent(prog->optInt("QUICKNESS", 0));
  int aerialCount   = ziruCountFromSpent(prog->optInt("AERIAL", 0));
  int defCount      = ziruCountFromSpent(prog->optInt("DEFENCE", 0));
  int gk1Count      = ziruCountFromSpent(prog->optInt("GK1", 0));
  int gk2Count      = ziruCountFromSpent(prog->optInt("GK2", 0));
  int gk3Count      = ziruCountFromSpent(prog->optInt("GK3", 0));

  int deltas[45] = {0};

  // SHOOT -> Finishing (20), Place Kicking (27), Curl (28)
  deltas[20] += shootCount;
  deltas[27] += shootCount;
  deltas[28] += shootCount;

  // PASS -> Low Pass (21), Lofted Pass (22)
  deltas[21] += passCount;
  deltas[22] += passCount;

  // DRIBBLE -> Ball Control (17), Dribble (18), Tight Possession (19)
  deltas[17] += dribbleCount;
  deltas[18] += dribbleCount;
  deltas[19] += dribbleCount;

  // QUICKNESS -> Offensive Awareness (14), Acceleration (35), Balance (36)
  deltas[14] += quickCount;
  deltas[35] += quickCount;
  deltas[36] += quickCount;

  // STRENGTH -> Speed (34), Kicking Power (37), Stamina (40)
  deltas[34] += strengthCount;
  deltas[37] += strengthCount;
  deltas[40] += strengthCount;

  // AERIAL -> Heading (23), Jump (38), Physical Contact (39)
  deltas[23] += aerialCount;
  deltas[38] += aerialCount;
  deltas[39] += aerialCount;

  // DEFENCE -> Defensive Awareness (15), Tackling (24), Aggression (25), Defensive Engagement (26)
  deltas[15] += defCount;
  deltas[24] += defCount;
  deltas[25] += defCount;
  deltas[26] += defCount;

  // GK1 -> GK Awareness (29), Jump (38)
  deltas[29] += gk1Count;
  deltas[38] += gk1Count;

  // GK2 -> GK Clearing (31), GK Reach (39)
  deltas[31] += gk2Count;
  deltas[39] += gk2Count;

  // GK3 -> GK Catching (30), GK Reflexes (32)
  deltas[30] += gk3Count;
  deltas[32] += gk3Count;

  for (int idx = 14; idx <= 40; idx++) {
    int baseVal = src->optInt(idx, 0);
    int newVal = std::min(105, baseVal + deltas[idx]);
    params->put(idx, newVal);
  }
}

ArrP z27ResolvePlayStyle(int64_t id, const ObjP& existing = nullptr) {
  if (existing) {
    ArrP ps = existing->optJSONArray("play_style");
    if (ps && ps->length() > 0 && ps->optInt(0, 0) > 0) return ps;
  }
  // 1. Try getZiruPlayerWrapper by exact ID
  ObjP w = getZiruPlayerWrapper(Value((long long)id), false);
  if (w && w->optJSONObject("gameplayer")) {
    ArrP ps = w->getJSONObject("gameplayer")->optJSONArray("play_style");
    if (ps && ps->length() > 0 && ps->optInt(0, 0) > 0) return ps;
  }
  // 2. Try getZiruPlayerWrapper by base ID (lower 24 bits)
  int32_t bid = (int32_t)(id & 0xFFFFFF);
  ObjP wBase = getZiruPlayerWrapperByBaseId(bid);
  if (wBase && wBase->optJSONObject("gameplayer")) {
    ArrP ps = wBase->getJSONObject("gameplayer")->optJSONArray("play_style");
    if (ps && ps->length() > 0 && ps->optInt(0, 0) > 0) return ps;
  }
  // 3. Try z27Entry() (CmdLogin.json)
  try {
    ArrP list = z27Entry()->optJSONArray("gameplayer_list");
    if (list) {
      for (int i = 0; i < list->length(); i++) {
        ObjP o = list->optJSONObject(i);
        if (!o) continue;
        ObjP gid = o->optJSONObject("gameplayer_id");
        if (!gid) continue;
        int64_t oid = gid->optLong("id", 0);
        if (oid == id || ((oid & 0xFFFFFF) == bid)) {
          ArrP ps = o->optJSONArray("play_style");
          if (ps && ps->length() > 0 && ps->optInt(0, 0) > 0) return ps;
        }
      }
    }
  } catch (...) {}
  // 4. Semantic default from position parameters so NO player is ever "Basic"
  int pos = 0;
  if (existing) {
    ArrP par = existing->optJSONArray("parameters");
    if (par && par->length() > 0) pos = par->optInt(0, 0);
  }
  ArrP fallback = newArr();
  if (pos == 1) { fallback->put(20); fallback->put(0); } // GK -> Offensive Goalkeeper
  else if (pos == 2 || pos == 3 || pos == 4) { fallback->put(15); fallback->put(0); } // CB/LB/RB -> Build Up
  else if (pos == 5 || pos == 6 || pos == 7 || pos == 8) { fallback->put(11); fallback->put(0); } // DMF/CMF/LMF/RMF -> Box-to-Box
  else if (pos == 9) { fallback->put(10); fallback->put(0); } // AMF -> Hole Player
  else if (pos == 10 || pos == 11) { fallback->put(6); fallback->put(0); } // LWG/RWG -> Prolific Winger
  else { fallback->put(1); fallback->put(0); } // CF/SS -> Goal Poacher
  return fallback;
}

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
        if (!w) {
          w = getZiruPlayerWrapperByBaseId((int32_t)(id & 0xFFFFFF));
        }
        ObjP base;
        if (w && (base = w->optJSONObject("gameplayer"))) {
          ArrP ib;
          if (!cur->optJSONArray("initial_booster_id_list") && (ib = base->optJSONArray("initial_booster_id_list"))) cur->put("initial_booster_id_list", ib);
          auto ps = base->get("play_style");
          if (ps && ps.isJSONArray() && ps.asJSONArray()->length() > 0 && ps.asJSONArray()->optInt(0, 0) > 0) cur->put("play_style", ps);
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
          int cp = ziruProgressionBudget(lim);
          if (serial > 0) {
            JStr pr = loadProgression(serial);
            if (pr && !pr->empty()) {
              cp -= Object::parse(*pr)->optInt("_spent_total", 0);
              if (cp < 0) cp = 0;
            }
          }
          li->put("customize_point", cp);
        }
        ArrP curPs = cur->optJSONArray("play_style");
        if (!curPs || curPs->length() == 0 || curPs->optInt(0, 0) <= 0) {
          cur->put("play_style", z27ResolvePlayStyle(id, cur));
        }
        z27ApplyAppearanceAndClub(cur);
        if (serial > 0) {
          ziruDb->savePlayer(serial, id, cur->toString());
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
      if (c && (cid = c->optJSONObject("coach_id")) && cid->optLong("id", 0) == id) return Object::parse(c->toString());
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
          if (isZiruSkillId(v) && n->length() < 5) n->put(v);
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
  Response r = newFixed(200, "application/x-msgpack", encodeClientResponse(jo ? Value(jo) : Value()));
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
    throw;
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
