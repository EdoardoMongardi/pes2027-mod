// ---- part 4 (OfflineServer.java ~1961-2805) ----

Response handleZiruMyLeagueResultV71() {
  try {
    ObjP req = decodeZiruRequestJson(ziruLastRequestBody);
    int home = 0, away = 0;
    int64_t rqid = 1;
    bool giveup = false;
    bool userHome = ziruMyLeagueUserIsHomeV74();
    if (req) {
      int64_t rq = req->optLong("rqid", 1);
      rqid = rq;
      ArrP sc = req->optJSONArray("score");
      if (sc && sc->length() >= 2) {
        ObjP a = sc->optJSONObject(0), b = sc->optJSONObject(1);
        int h = 0, w = 0;
        bool z = false;
        if (a) h = a->optInt("score_1st") + a->optInt("score_2nd") + a->optInt("score_ex1st") + a->optInt("score_ex2nd");
        if (b) {
          w = b->optInt("score_1st") + b->optInt("score_2nd") + b->optInt("score_ex1st") + b->optInt("score_ex2nd");
          ObjP mr;
          if (req->optString("intentional_give_up", "NO") == "YES" && (mr = req->optJSONObject("match_result")) &&
              mr->optString("abnormalend_reason", "NORMAL") == "NORMAL_GIVEUP" && h == 0 && w == 0) {
            z = true;
            if (userHome) { h = 0; w = 3; } else { h = 3; w = 0; }
          }
        }
        std::string rs = str(rq);
        JStr last = loadMeta("ziru_myleague_last_result_rqid");
        home = h; away = w; giveup = z;
        if (!(last && rs == *last)) {
          ziruDb->saveMeta("ziru_myleague_last_result_rqid", rs);
          ziruDb->saveMeta("ziru_myleague_last_home_score", str(h));
          ziruDb->saveMeta("ziru_myleague_last_away_score", str(w));
          ziruDb->saveMeta("ziru_myleague_last_gameend_reason", z ? "NORMAL_GIVEUP" : "NORMAL_END");
          std::string res = "DRAW";
          if (z) res = "LOSE";
          else if (userHome) { if (h > w) res = "WIN"; else if (h < w) res = "LOSE"; }
          else if (h < w) res = "WIN";
          else if (h > w) res = "LOSE";
          ziruDb->saveMeta("ziru_myleague_last_result", res);
          ziruMlRecordRealMatchEventsV85(req);
          ziruMyLeagueCommitStateV74(res, h, w, userHome);
          ziruMlSimulateCurrentFixtureV83(h, w);
          if (res == "WIN") {
            int p = ziruLeagueLoadInt("ziru_myleague_point", 9760);
            int rw = ziruMyLeagueRewardForCpuLevelV72();
            ziruLeagueSaveInt("ziru_myleague_point", p + rw);
            ziruLeagueSaveInt("ziru_myleague_last_added_point", rw);
          } else {
            ziruLeagueSaveInt("ziru_myleague_last_added_point", 0);
          }
          int f = ziruLeagueLoadInt("ziru_myleague_fixture", 1) + 1;
          int mx = ziruMyLeagueMaxFixtureV80();
          if (f > mx) f = mx;
          ziruLeagueSaveInt("ziru_myleague_fixture", f);
        }
      }
    }
    ObjP jo = loadZiruJsonAssetObject("responses/ZIRU_ML_GAME_RESULT.json");
    patchZiruMyLeagueCommonState(jo);
    patchZiruMyLeaguePostMatchSeasonV82(jo);
    if (req) {
      nn(jo)->put("rqid", (long long)rqid);
      ObjP ge = jo->optJSONObject("game_end_result");
      ObjP mr;
      if (ge && (mr = ge->optJSONObject("match_result"))) {
        std::string res = "DRAW";
        if (giveup) res = "LOSE";
        else if (userHome) { if (home > away) res = "WIN"; else if (home < away) res = "LOSE"; }
        else if (home < away) res = "WIN";
        else if (home > away) res = "LOSE";
        mr->put("game_res", res);
        std::string ge2 = giveup ? "NORMAL_GIVEUP" : "NORMAL_END";
        mr->put("gameend_reason", ge2);
        mr->put("abnormalend_reason", ge2);
        ObjP sd = mr->optJSONObject("score_data");
        if (sd) { sd->put("home_score", home); sd->put("away_score", away); }
      }
    }
    return serveObjectAsEncryptedMsgPack(jo);
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V73_ML_RESULT_ERR] ") + e.what());
    return handleZiruLeagueStatic("responses/ZIRU_ML_GAME_RESULT.json");
  }
}

Response handleZiruMyLeagueScheduleV74() {
  try {
    ObjP sch = nn(loadZiruMyLeagueScheduleV79());
    ArrP list = sch->optJSONArray("schedule_list");
    int team = ziruLeagueLoadInt("ziru_myleague_team_id", 100);
    if (list) {
      for (int i = 0; i < list->length(); i++) {
        ObjP fx = list->optJSONObject(i);
        if (!fx) continue;
        int i2 = i + 1;
        if (loadMeta(ziruMyLeagueFixtureKeyV74(i2, "result"))) {
          JStr h = loadMeta(ziruMyLeagueFixtureKeyV74(i2, "home"));
          JStr a = loadMeta(ziruMyLeagueFixtureKeyV74(i2, "away"));
          if (h && a) {
            int hp = parseInt(*h), ap = parseInt(*a);
            ArrP ml = fx->optJSONArray("match_list");
            if (ml) {
              for (int i3 = 0; i3 < ml->length(); i3++) {
                ObjP m = ml->optJSONObject(i3);
                if (!m) continue;
                if (m->optInt("home_team_id") == team || m->optInt("away_team_id") == team) {
                  m->put("home_goals", hp);
                  m->put("away_goals", ap);
                  break;
                }
              }
            }
          }
        }
      }
    }
    patchZiruMyLeagueScheduleScoresV84(sch);
    ObjP req = decodeZiruRequestJson(ziruLastRequestBody);
    if (req) sch->put("rqid", (long long)req->optLong("rqid", 1));
    sch->put("result", "NOERR");
    sch->put("msgid", "CMD_GET_SEASON_SCHEDULE");
    return serveObjectAsEncryptedMsgPack(sch);
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V74_SCHEDULE_ERR] ") + e.what());
    return handleZiruLeagueStatic("responses/ZIRU_ML_SCHEDULE_113.json");
  }
}

Response handleZiruMyLeagueSpecialSquadV71() {
  try {
    ObjP req = decodeZiruRequestJson(ziruLastRequestBody);
    ObjP sq;
    if (req && (sq = req->optJSONObject("special_squad_data"))) {
      hydrateZiruMyLeagueCoachV73(sq);
      sq->put("initial_settings", "YES");
      ziruDb->saveMeta("ziru_myleague_special_squad", sq->toString());
    }
    return handleZiruLeagueStatic("responses/ZIRU_ML_SET_SPECIAL_SQUAD.json");
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V71_SPECIAL_SQUAD_ERR] ") + e.what());
    return handleZiruLeagueStatic("responses/ZIRU_ML_SET_SPECIAL_SQUAD.json");
  }
}

Response handleZiruMyLeagueStartGameV71() {
  try {
    ObjP jo = loadZiruJsonAssetObject("responses/ZIRU_ML_START_GAME.json");
    patchZiruMyLeagueCommonState(jo);
    ObjP req = decodeZiruRequestJson(ziruLastRequestBody);
    if (req) {
      nn(jo)->put("rqid", (long long)req->optLong("rqid", 1));
      int64_t gid = req->optLong("game_id", 0);
      if (gid != 0) {
        jo->put("game_id", (long long)gid);
        ziruDb->saveMeta("ziru_myleague_game_id", str(gid));
      }
    }
    JStr m = loadMeta("ziru_myleague_special_squad");
    if (m) {
      ObjP sq = Object::parse(*m);
      hydrateZiruMyLeagueCoachV73(sq);
      ObjP sd = sq->optJSONObject("squad_data");
      ObjP home;
      if (sd && (home = nn(jo)->optJSONObject("squad_data_home"))) {
        ArrP pl = sd->optJSONArray("player_list");
        if (pl) home->put("player_list", pl);
        ArrP co = sd->optJSONArray("coach");
        ObjP c0;
        if (co && (c0 = co->optJSONObject(0))) home->put("coach", c0);
        ObjP gp = sd->optJSONObject("squad_gameplan");
        if (gp) home->put("squad_gameplan", gp);
      }
    }
    return serveObjectAsEncryptedMsgPack(jo);
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V71_START_GAME_ERR] ") + e.what());
    return handleZiruLeagueStatic("responses/ZIRU_ML_START_GAME.json");
  }
}

Response handleZiruProtocolAckV81(const JStr& msg) {
  try {
    int64_t rq = 0;
    JStr m = msg;
    ObjP req;
    if (ziruLastRequestBody && (req = decodeZiruRequestJson(ziruLastRequestBody))) {
      rq = req->optLong("rqid", 0);
      if (!m) m = req->optString("msgid", "GENERIC");
    }
    if (!m) m = std::string("GENERIC");
    ObjP jo = newObj();
    jo->put("result", "NOERR");
    jo->put("rqid", (long long)rq);
    jo->put("msgid", *m);
    return serveObjectAsEncryptedMsgPack(jo);
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V81_ACK_ERR] ") + e.what());
    return serveJsonAsEncryptedMsgPack("responses/generic.json");
  }
}

std::optional<Response> handleZiruSellPlayer() {
  int64_t rq = 0;
  try {
    ObjP req = decodeZiruRequestJson(ziruLastRequestBody);
    if (req) {
      rq = req->optLong("rqid");
      ArrP l = req->optJSONArray("gameplayer_id_list");
      if (l) {
        int n = l->length();
        for (int i = 0; i < n; i++) {
          ObjP o = l->optJSONObject(i);
          if (o) {
            int64_t s = o->optLong("serial");
            if (s > 0) markZiruPlayerSold(s);
          }
        }
      }
    }
    ObjP jo = newObj();
    jo->put("gain_gp", 0);
    jo->put("gp", 18255257);
    jo->put("result", "NOERR");
    jo->put("rqid", (long long)rq);
    jo->put("msgid", "CMD_SELL_MYCLUB_GAMEPLAYER");
    return serveObjectAsEncryptedMsgPack(jo);
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V70E_SELL_ERR] ") + e.what());
    return std::nullopt;
  }
}

std::optional<Response> handleZiruSellPlayerEarly(Request& r) {
  try {
    // InputStream.available(): NanoHTTPD's stream reports buffered bytes; with a fully received body this is the remaining length.
    size_t avail = r.body.size() - r.readPos;
    if (avail > 0) {
      Bytes b(avail);
      size_t got = 0;
      while (got < avail) {
        size_t k = r.read(b.data() + got, avail - got);
        if (k == 0) break;
        got += k;
      }
      if (got == avail) ziruLastRequestBody = b;
    }
    return handleZiruSellPlayer();
  } catch (const std::exception& e) {
    return handleZiruSellPlayer();
  }
}

Response handleZiruSquadAck() { return z27SquadAck(); }

Response handleZiruStartupAck(const std::string& msg) {
  try {
    int64_t rq = 0;
    ObjP req;
    if (ziruLastRequestBody && (req = decodeZiruRequestObject(ziruLastRequestBody))) rq = req->optLong("rqid", 0);
    ObjP jo = newObj();
    if (msg == "CMD_GET_AGE_GATE_REQUIREMENTS") {
      jo->put("is_eula_country", "YES");
      jo->put("country_age_kind", "BIRTH_YEAR_MONTH");
    }
    jo->put("result", "NOERR");
    jo->put("rqid", (long long)rq);
    jo->put("msgid", msg);
    return serveObjectAsEncryptedMsgPack(jo);
  } catch (const std::exception& e) {
    return serveJsonAsEncryptedMsgPack("responses/generic.json");
  }
}

Response handleZiruTrackrecordMinimal() {
  logRequest("ZIRU_V81_PROTOCOL", "[ZIRU_V81_TRACKRECORD_ACK] exact rqid");
  return handleZiruProtocolAckV81(JStr("CMD_SET_TRACKRECORD"));
}

void hydrateZiruMyLeagueCoachV73(const ObjP& jo) {
  if (!jo) return;
  try {
    ObjP sd = jo->optJSONObject("squad_data");
    ArrP co;
    ObjP c0, cid;
    if (!sd || !(co = sd->optJSONArray("coach")) || !(c0 = co->optJSONObject(0)) || !(cid = c0->optJSONObject("coach_id"))) return;
    int64_t id = cid->optLong("id", 0);
    ObjP res;
    if (id == 0 || !(res = resolveZiruCoachFromLoginV73(id))) return;
    co->put(0, res);
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V73_ML_COACH_HYDRATE_ERR] ") + e.what());
  }
}

bool isZiruLocalCommentaryInstalled() {
  std::string dir = cfg.dataDir + "/Download";
  mkdir(dir.c_str(), 0755);
  struct stat st;
  std::string f = dir + "/dt510_eng_mobile_all.cpk";
  if (stat(f.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
    logRequest("ZIRU_COMMENTARY_V59", "[ZIRU_V59_COMMENTARY_MISSING] put dt510_eng_mobile_all.cpk in files/Download");
    return false;
  }
  logRequest("ZIRU_COMMENTARY_V59", "[ZIRU_V59_COMMENTARY_FOUND] size=" + str((int64_t)st.st_size));
  return true;
}

bool isZiruPlayerSold(int64_t s) {
  JStr m = loadMeta("sold_player_" + str(s));
  return m && *m == "1";
}

ObjP loadZiruJsonAssetObject(const std::string& name) {
  try {
    return Object::parse(readAsset(name));
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V46_LOAD_JSON_ERR] ") + name + " " + e.what());
    return nullptr;
  }
}

ObjP loadZiruMyLeagueScheduleV79() {
  int c = ziruLeagueLoadInt("ziru_myleague_compe_unit_id", 113);
  return loadZiruJsonAssetObject("responses/ZIRU_ML_SCHEDULE_" + str(c) + ".json");
}

void logAlmanacAgentRequestBody(Request& r) {
  ziruLastRequestBody.reset();
  try {
    auto it = r.headers.find("content-length");
    int n;
    if (it == r.headers.end() || (n = parseInt(it->second)) <= 0) {
      zs::logw("OFFLINE_SERVER", "[ALMANAC_REQ_WARN] no usable Content-Length");
      return;
    }
    int got;
    Bytes b = readBody(r, n, got);
    if (got != n) { zs::logw("OFFLINE_SERVER", "[ALMANAC_REQ_WARN] partial body"); return; }
    ziruLastRequestBody = b;
    const std::string& uri = r.uri;
    if (contains(uri, "MYCLUB_AGENT") || contains(uri, "PROCURABLE_GAMEPLAYERLIST")) decodeZiruAgentRequest(b);
    if (contains(uri, "USE_MYCLUB_ITEM")) decodeZiruMyclubItemRequest(b);
    if (contains(uri, "SET_MYCLUB_SQUAD_INFO")) decodeAndPersistZiruSquadRequest(b);
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ALMANAC_REQ_ERR] request probe failed ") + e.what());
  }
}

void markZiruPlayerSold(int64_t s) {
  ziruDb->saveMeta("sold_player_" + str(s), "1");
  ziruDb->deletePlayerState(s);
  logRequest("ZIRU_SELL_V28", "[ZIRU_SELL_V28_FIX1_MARK] serial=" + str(s));
}

void normalizeZiruAllLoginPlayers(const ObjP& jo) {
  try {
    ObjP ei = jo->optJSONObject("entry_info");
    ArrP list;
    if (ei && (list = ei->optJSONArray("gameplayer_list"))) {
      for (int i = 0; i < list->length(); i++) {
        ObjP p = list->optJSONObject(i);
        ObjP r;
        if (p && (r = refreshZiruGameplayerFromCanonical(p))) list->put(i, r);
      }
    }
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_LEVEL_V18_ERR] ") + e.what());
  }
}

std::string livedataBaseUrl = "https://127.0.0.1:8085/livedata/";

void patchLocalLivedataUrls(const ObjP& jo) {
  try {
    ObjP li = jo->optJSONObject("livedata_info");
    ArrP list;
    if (!li || !(list = li->optJSONArray("livedata_list"))) return;
    for (int i = 0; i < list->length(); i++) {
      ObjP o = list->optJSONObject(i);
      if (!o) continue;
      std::string name = o->optString("name", "");
      if (name.empty()) continue;
      if (assets.exists("livedata/" + name)) o->put("url", livedataBaseUrl + name);
    }
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ALMANAC_V8] LiveData URL patch failed ") + e.what());
  }
}

void patchZiruLeagueHistoryIntoResult(const ObjP& jo) {
  if (!jo) return;
  try {
    ObjP li = jo->optJSONObject("league_info");
    ObjP rmr;
    if (!li || !(rmr = li->optJSONObject("round_match_result"))) return;
    ArrP h = Array::parse(ziruDb->loadLeagueHistory());
    rmr->put("match_history_list", h);
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V63_RESULT_HISTORY_INJECT_ERR] ") + e.what());
  }
}

void patchZiruLeagueInfo(const ObjP& jo) { z27Info(jo); }

void patchZiruMyLeagueAddedPointV83(const ObjP& jo) {
  if (!jo) return;
  int p = ziruLeagueLoadInt("ziru_myleague_last_added_point", 0);
  ArrP a = newArr();
  if (p > 0) {
    ObjP e = newObj();
    e->put("reason", "MATCH_WIN");
    e->put("myleague_point", p);
    a->put(e);
  }
  jo->put("added_point_list", a);
}

void patchZiruMyLeagueCommonState(const ObjP& jo) {
  if (!jo || !ziruMyLeagueIsActive()) return;
  try {
    int p = ziruLeagueLoadInt("ziru_myleague_point", 9760);
    if (jo->has("myleague_point")) jo->put("myleague_point", p);
    if (jo->has("point")) jo->put("point", p);
    int team = ziruLeagueLoadInt("ziru_myleague_team_id", 100);
    if (jo->has("team_id")) jo->put("team_id", team);
    int compe = ziruLeagueLoadInt("ziru_myleague_compe_unit_id", 113);
    if (jo->has("league_compe_unit_id")) {
      jo->put("league_compe_unit_id", compe);
      if (jo->has("compe_unit_id")) jo->put("compe_unit_id", compe);
    }
    int f = ziruLeagueLoadInt("ziru_myleague_fixture", 1);
    if (jo->has("fixture")) jo->put("fixture", f);
    JStr m = loadMeta("ziru_myleague_special_squad");
    if (!m) {
      ObjP sd = jo->optJSONObject("special_dream_squad_data");
      if (sd) sd->put("rental_team_id", team);
      return;
    }
    if (jo->has("special_dream_squad_data")) {
      ObjP s = Object::parse(*m);
      s->put("initial_settings", "YES");
      jo->put("special_dream_squad_data", s);
    }
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V71_COMMON_PATCH_ERR] ") + e.what());
  }
}

void patchZiruMyLeagueLeaderboardsV85(const ObjP& jo) {
  if (!jo) return;
  jo->put("goal_ranking", buildZiruMlLeaderboardV85("ziru_myleague_goal_stats_json"));
  jo->put("assist_ranking", buildZiruMlLeaderboardV85("ziru_myleague_assist_stats_json"));
}

void patchZiruMyLeagueOpponentV82(const ObjP& jo) {
  if (!jo) return;
  try {
    int f = ziruLeagueLoadInt("ziru_myleague_fixture", 1);
    int team = ziruLeagueLoadInt("ziru_myleague_team_id", 100);
    ArrP sl = nn(loadZiruMyLeagueScheduleV79())->optJSONArray("schedule_list");
    ObjP fx;
    ArrP ml;
    if (!sl || !(fx = sl->optJSONObject(f - 1)) || !(ml = fx->optJSONArray("match_list"))) return;
    for (int i = 0; i < ml->length(); i++) {
      ObjP m = ml->optJSONObject(i);
      if (!m) continue;
      int h = m->optInt("home_team_id"), a = m->optInt("away_team_id");
      int opp;
      std::string side;
      if (h == team) { opp = a; side = "AWAY"; }
      else if (a == team) { opp = h; side = "HOME"; }
      else continue;
      jo->put("side", side);
      ObjP td = jo->optJSONObject("team_data");
      if (td) td->put("base_team_id", opp);
      return;
    }
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V82_ML_OPPONENT_ERR] ") + e.what());
  }
}

void patchZiruMyLeaguePostMatchSeasonV82(const ObjP& jo) {
  if (!jo) return;
  ObjP ge = jo->optJSONObject("game_end_result");
  ObjP em, sm;
  if (!ge || !(em = ge->optJSONObject("extra_match_result")) || !(sm = em->optJSONObject("season_match_result"))) return;
  patchZiruMyLeagueRankingV79(sm);
}

void patchZiruMyLeagueRankingV79(const ObjP& jo) {
  if (!jo) return;
  try {
    ArrP teams = ziruMyLeagueCompetitionTeamsV80(ziruLeagueLoadInt("ziru_myleague_compe_unit_id", 113));
    if (teams) {
      int user = ziruLeagueLoadInt("ziru_myleague_team_id", 100);
      ArrP rows = newArr();
      for (int i = 0; i < teams->length(); i++) appendZiruMyLeagueRankRowV79(rows, teams->optInt(i), user);
      sortZiruMyLeagueRankingV83(rows);
      jo->put("ranking", rows);
    }
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V80_RANK_ERR] ") + e.what());
  }
}

void patchZiruMyLeagueScheduleScoresV84(const ObjP& jo) {
  if (!jo) return;
  try {
    int f = ziruLeagueLoadInt("ziru_myleague_fixture", 1);
    int team = ziruLeagueLoadInt("ziru_myleague_team_id", 100);
    ArrP sl = jo->optJSONArray("schedule_list");
    if (!sl) return;
    for (int i2 = 0; i2 < sl->length(); i2++) {
      int i = i2 + 1;
      if (i >= f) break;
      ObjP fx = sl->optJSONObject(i2);
      ArrP ml;
      if (fx && (ml = fx->optJSONArray("match_list"))) {
        for (int i3 = 0; i3 < ml->length(); i3++) {
          ObjP m = ml->optJSONObject(i3);
          if (!m) continue;
          int h = m->optInt("home_team_id"), a = m->optInt("away_team_id");
          if (h != team && a != team) {
            m->put("home_goals", ((h + a) + i) % 4);
            m->put("away_goals", (((h * 3) + (a * 2)) + i) % 4);
          }
        }
      }
    }
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V84_SCHEDULE_SCORE_ERR] ") + e.what());
  }
}

void patchZiruMyLeagueSeasonStateV74(const ObjP& jo) {
  if (!jo) return;
  try {
    int f = ziruLeagueLoadInt("ziru_myleague_fixture", 1);
    int team = ziruLeagueLoadInt("ziru_myleague_team_id", 100);
    ArrP sl = nn(loadZiruMyLeagueScheduleV79())->optJSONArray("schedule_list");
    ObjP fx;
    if (sl && (fx = sl->optJSONObject(f - 1))) {
      jo->put("is_boss_match", fx->optString("is_boss_match", "NO"));
      ArrP ml = fx->optJSONArray("match_list");
      if (ml) {
        jo->put("auto_match_list", ml);
        for (int i = 0; i < ml->length(); i++) {
          ObjP m = ml->optJSONObject(i);
          if (!m) continue;
          if (m->optInt("home_team_id") != team) {
            if (m->optInt("away_team_id") == team) {
              jo->put("user_side", "AWAY");
              jo->put("opponent_team_id", m->optInt("home_team_id"));
              break;
            }
          } else {
            jo->put("user_side", "HOME");
            jo->put("opponent_team_id", m->optInt("away_team_id"));
            break;
          }
        }
      }
    }
    JStr h = loadMeta("ziru_myleague_history_json");
    if (h) jo->put("match_history_list", Array::parse(*h));
    ArrP rk = jo->optJSONArray("ranking");
    if (rk) {
      for (int i2 = 0; i2 < rk->length(); i2++) {
        ObjP r = rk->optJSONObject(i2);
        if (r && r->optInt("team_id") == team) {
          r->put("winning_points", ziruLeagueLoadInt("ziru_myleague_table_points", 0));
          r->put("wins", ziruLeagueLoadInt("ziru_myleague_table_wins", 0));
          r->put("draws", ziruLeagueLoadInt("ziru_myleague_table_draws", 0));
          r->put("losses", ziruLeagueLoadInt("ziru_myleague_table_losses", 0));
          int gf = ziruLeagueLoadInt("ziru_myleague_table_gf", 0);
          r->put("goals", gf);
          int ga = ziruLeagueLoadInt("ziru_myleague_table_ga", 0);
          r->put("goals_against", ga);
          r->put("diff", gf - ga);
          return;
        }
      }
    }
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V74_PATCH_ERR] ") + e.what());
  }
}
