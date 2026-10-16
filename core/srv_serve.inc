// ---- serve() routing and serveJsonAsEncryptedMsgPack (OfflineServer.java 4722-5075) ----

Response serveJsonAsEncryptedMsgPack(const std::string& name) {
  std::string text = readAsset(name);  // IOException propagates (caller decides)
  std::string trimmed = text;
  size_t b = 0;
  while (b < trimmed.size() && (unsigned char)trimmed[b] <= ' ') b++;
  Value root;
  if (b < trimmed.size() && trimmed[b] == '[') root = Value(Array::parse(text));
  else root = Value(Object::parse(text));

  ObjP jo = root.t == Value::OBJ ? root.o : nullptr;
  ObjP req;
  if (jo && ziruLastRequestBody && (req = decodeZiruRequestJson(ziruLastRequestBody))) {
    jo->put("rqid", (long long)req->optLong("rqid", 0));
    std::string msgid = req->optString("msgid", "");
    if (!msgid.empty()) jo->put("msgid", msgid);
  }
  if (jo && (name == "responses/CmdLogin.json" || name == "responses/get_user_eula_info.json")) {
    if (name == "responses/CmdLogin.json") patchLocalLivedataUrls(jo);
    ObjP eula = jo->optJSONObject("user_eula_info");
    if (eula) {
      eula->put("eula_privacy_action", "NONE");
      eula->put("eula_country_age_kind", "NONE");
      eula->put("eula_ip_country_a3", "VNM");
      eula->put("eula_select_country_a3", "VNM");
    }
    jo->put("select_country_privacy_action", "NONE");
    jo->put("country_status", "NO_CHANGE");
  }
  if (jo && (name == "responses/CmdGetServerEnv.json" || name == "responses/GateInfo.json")) {
    jo->put("ip_country_code", "VNM");
  }
  if (jo && (name == "responses/get_myclub_commentary_info.json" || name == "responses/ZIRU_GET_COMMENTARY_DATA.json"))
    patchIosCommentaryData(jo);
  if (jo) {
    if (name == "responses/CmdGetMyclubProcurableGameplayerlist.json") applyZiruProcurableList(jo);
    if (name == "responses/CmdGetMyclubAgentBoxInfo.json") ziruPatchAgentBoxInfo(jo);
    if (name == "responses/CmdResetMyclubAgent.json") applyZiruResetBox(jo);
    if (name == "responses/CmdUseMyclubAgent.json") {
      applyZiruDynamicRecruit(jo);
      ObjP ci = jo->optJSONObject("coin_info");
      if (ci) ci->put("coin", 9999999);
    }
    ArrP gl;
    if (name == "responses/CmdUseMyclubAgent.json" && (gl = jo->optJSONArray("gameplayer_list"))) {
      zs::Prefs& prefs = sp("almanac_state");
      std::string s = prefs.getString("recruited_players", "");
      std::string out;
      if (s.empty()) out = gl->toString();
      else {
        ArrP acc = Array::parse(s);
        for (int i = 0; i < gl->length(); i++) { ObjP o = gl->optJSONObject(i); if (o) acc->put(o); }
        out = acc->toString();
      }
      prefs.edit().putString("recruited_players", out).commit();
    }
    std::string rp;
    if (name == "responses/CmdLogin.json" && !(rp = sp("almanac_state").getString("recruited_players", "")).empty()) {
      ArrP acc = Array::parse(rp);
      ObjP ei = jo->optJSONObject("entry_info");
      ArrP l2;
      if (ei && (l2 = ei->optJSONArray("gameplayer_list"))) {
        for (int i = 0; i < acc->length(); i++) {
          ObjP o = acc->optJSONObject(i);
          ObjP gp;
          if (o && (gp = o->optJSONObject("gameplayer"))) l2->put(gp);
        }
      }
    }
  }
  if (jo && name == "responses/CMD_USE_MYCLUB_ITEM.json") {
    applyZiruPositionTrainingResponse(jo);
    applyZiruBoosterTrainingResponse(jo);
    applyZiruSkillTrainingResponse(jo);
  }
  if (jo && name == "responses/CmdLogin.json") filterZiruSoldPlayersFromLogin(jo);
  if (jo && name == "responses/CmdLogin.json") normalizeZiruAllLoginPlayers(jo);
  if (jo && name == "responses/CmdLogin.json") applyZiruSquadStateToLogin(jo);
  if (jo && (name == "responses/CmdGetMyclubAgentlist.json" || name == "responses/get_myclub_mainmenu_info.json")) applyZiruRemainingCounts(jo);
  if (jo && name == "responses/get_myclub_mainmenu_info.json") {
    jo->put("gp", 99999999999LL);
    ObjP ci = jo->optJSONObject("coin_info");
    if (ci) ci->put("coin", 9999999LL);
    ArrP bl = jo->optJSONArray("banner_list");
    if (bl) {
      for (int i = 0; i < bl->length(); i++) {
        ObjP b = bl->optJSONObject(i);
        if (b && b->optString("announcement_type", "") == "AGENT") {
          b->put("announcement_type", "NONE");
          b->put("entity_id", 0);
        }
      }
    }
  }
  if (jo && name == "responses/get_user_eula_info.json") {
    ObjP uei = jo->optJSONObject("user_eula_info");
    if (uei) {
      uei->put("eula_privacy_action", "NONE");
      uei->put("eula_country_age_kind", "NONE");
    }
    jo->put("select_country_privacy_action", "NONE");
    jo->put("country_status", "NO_CHANGE");
  }
  if (jo) {
    z27Restore(jo);
    z27RepairLoginRoster(jo);
  }
  Response r = newFixed(200, "application/x-msgpack", zc::encodeResponse(root));
  r.headers["Connection"] = "close";
  return r;
}

static const char* routeAsset(const std::string& uri) {
  struct R { const char* k; const char* v; };
  static const R first[] = {
      {"GateInfo", "GateInfo.json"}, {"GET_SERVER_ENV", "CmdGetServerEnv.json"}, {"CmdGetServerEnv.php", "CmdGetServerEnv.json"},
      {"CmdGetCountryList.php", "GetCountryList.json"}, {"CmdGetProductList.php", "get_product_list.json"},
      {"CmdGetMyclubMainmenuInfo.php", "get_myclub_mainmenu_info.json"}, {"CmdGetMyclubCoachContractNorma.php", "get_myclub_coaches_norma.json"},
      {"CmdGetMyclubCommentaryInfo.php", "get_myclub_commentary_info.json"}, {"CmdSetMyclubEntryInfo.php", "set_myclub_entry_info.json"},
      {"CmdLogin.php", "CmdLogin.json"}, {"CmdCreateUser.php", "CmdCreateUser.json"}, {"CmdGetMyclubEntryInfo.php", "CmdGetMyclubEntryInfo.json"},
      {"CmdCheckString.php", "CmdCheckString.json"}, {"CmdSetMyclubUserInfo.php", "CmdSetMyclubUserInfo.json"},
      {"CmdGetMyclubAchievementlist.php", "CmdGetMyclubAchievementlist.json"},
      {"CmdSetMyclubTutorialAchievementInfo.php", "CmdSetMyclubTutorialAchievementInfo.json"}, {"CmdExtendMyclubCoach.php", "CmdExtendMyclubCoach.json"},
      {"CmdExtendMyclubGameplayer.php", "CmdExtendMyclubGameplayer.json"}, {"CmdGetMyclubPresentlist.php", "CmdGetMyclubPresentlist.json"},
      {"COUNTRY", "GetCountryList.json"}, {"PRODUCT", "get_product_list.json"}, {"MAINTENANCE", "get_maintenance_info.json"},
      {"LOGIN", "CmdLogin.json"}, {"CREATE_USER", "CmdCreateUser.json"}, {"ENTRY_INFO", "CmdGetEntryInfo.json"},
      {"AGENT_BOX", "CmdGetMyclubAgentBoxInfo.json"}, {"AGENTLIST", "CmdGetMyclubAgentlist.json"},
      {"COACH_NORMA", "CmdGetMyclubCoachContractNorma.json"}, {"COMMENTARY", "get_myclub_commentary_info.json"},
      {"MARKET_AGENT", "get_myclub_market_agentlist.json"}, {"PRESENTLIST", "get_myclub_presentlist.json"},
      {"PROCURABLE", "CmdGetMyclubProcurableGameplayerlist.json"}, {"RESET_MYCLUB_AGENT", "CmdResetMyclubAgent.json"},
      {"USE_MYCLUB_AGENT", "CmdUseMyclubAgent.json"}, {"USE_MYCLUB_ITEM", "CMD_USE_MYCLUB_ITEM.json"},
      {"SET_MYCLUB_SQUAD_INFO", "CMD_SET_MYCLUB_SQUAD_INFO.json"},
      {"VSCOM", "get_myclub_vscom_opponent.json"}, {"RECOVER_ENERGY", "CmdRecoverEnergy.json"}, {"DOWNLOAD_STATS", "CmdSendDownloadStatsData.json"},
      {"HEARTBEAT", "CmdSendHeartbeat.json"}, {"MATCH_TEAM", "CmdSendMatchTeamInfo.json"}, {"PLAYLOG", "CmdSendPlaylog.json"},
      {"DEVICETOKEN", "CmdSetDevicetoken.json"}, {"MATCH_RESULT", "CmdSetMatchResult.json"}, {"LANGUAGE", "CmdSetMyclubLanguage.json"},
      {"LOCK_GAMEPLAYER", "CmdSetMyclubLockGameplayer.json"}, {"MAIN_SQUAD", "CmdSetMyclubMainSquad.json"},
      {"SEASON_UPDATE", "CmdSetMyclubSeasonUpdateInfo.json"}, {"SQUAD_INFO", "CmdSetMyclubSquadInfo.json"},
      {"SQUAD_NAME", "CmdSetMyclubSquadName.json"}, {"TUTORIAL", "CmdSetMyclubTutorialAchievementInfo.json"},
      {"MYCLUB_USER", "set_myclub_user_info.json"}, {"USER_UPDATE", "set_user_update_info.json"}, {"START_MATCH", "cmd_start_match.json"},
      {"USE_AGENT", "CmdUseMyclubAgent.json"}, {"AUCTION_WATCH", "cmd_add_myclub_auction_watch_list.json"},
      {"AUCTION_UPDATE", "cmd_get_myclub_auction_update_info.json"}, {"MARKET_COACH", "cmd_get_myclub_market_coachlist.json"},
      {"NEWSLIST", "cmd_get_myclub_newslist.json"}, {"EXTEND_GAMEPLAYER", "extend_myclub_gameplayer.json"},
      {"QUALITYCHECK", "gamerelay_qualitycheck_list.json"}, {"ADVERTISEMENT", "get_advertisement.json"}, {"MAILBOX", "get_mailbox_info.json"},
      {"CREATIVE", "get_mainmenu_match_creative_info.json"}, {"FRIEND_PENDING", "get_myclub_friend_pending_list.json"},
      {"MAINMENU", "get_myclub_mainmenu_info.json"}, {"USER_DETAIL", "get_myclub_user_detail.json"}, {"ONLINE_MATCH", "get_online_match_info.json"},
      {"PATHTOGLORY", "get_pathtoglory_info.json"}, {"SEASON_INFO", "get_season_info.json"},
      {"RENTALPLAYER", "get_season_point_rentalplayer_list.json"}, {"STADIUM", "get_stadium_data.json"}, {"TEAM_LIST", "get_useable_team_list.json"},
      {"EULA", "get_user_eula_info.json"}, {"AUCTIONLIST", "getauctionlist.json"}, {"ACHIEVEMENT", "set_myclubuser_achieveentlist.json"},
      {"GET_MYCLUB_CONTRACT_INFO", "get_myclub_contract_info.json"}, {"START_GAME", "start_game.json"},
  };
  for (const R& r : first)
    if (uri.find(r.k) != std::string::npos) return r.v;
  return nullptr;
}

Response serveInner(Request& r) {
  const std::string& uri = r.uri;
  if (startsWith(uri, "/livedata/")) return serveLivedataAsset(uri.substr(1));
  if (contains(uri, "CMD_SELL_MYCLUB_GAMEPLAYER")) {
    auto x = handleZiruSellPlayerEarly(r);
    if (!x) return newFixedText(500, "SERVER INTERNAL ERROR: Serve() returned a null response.");
    return *x;
  }
  logRequest(uri, r.method);
  if (contains(uri, "CMD_SET_USER_EULA_INFO") || contains(uri, "CmdSetUserEulaInfo")) return handleZiruStartupAck("CMD_SET_USER_EULA_INFO");
  if (contains(uri, "CMD_USE_MYCLUB_CAREERPLAN")) return handleZiruCareerplan(r);
  logAlmanacAgentRequestBody(r);
  traceZiruUserCompeV87(uri);
  auto zr = z27Route(uri);
  if (zr) return *zr;
  if (contains(uri, "CMD_GET_MAINMENU_MATCH_CREATIVE_INFO")) return handleZiruMyLeagueMainmenuCreativeV78();
  if (contains(uri, "CMD_SET_SEASON_ENTER")) return handleZiruMyLeagueEnterV71();
  if (contains(uri, "CMD_GET_SEASON_INFO")) return handleZiruMyLeagueInfoV71();
  if (contains(uri, "CMD_GET_SEASON_SCHEDULE")) return handleZiruMyLeagueScheduleV74();
  if (contains(uri, "CMD_GET_SEASON_POINT_ITEM_LIST")) return handleZiruLeagueStatic("responses/ZIRU_ML_POINT_ITEM_LIST.json");
  if (contains(uri, "CMD_EXCHANGE_SEASON_POINT_ITEM")) return handleZiruMyLeagueExchangeItemV71();
  if (contains(uri, "CMD_GET_SEASON_POINT_RENTALPLAYER_LIST")) return handleZiruLeagueStatic("responses/ZIRU_ML_RENTAL_LIST.json");
  if (contains(uri, "CMD_EXCHANGE_SEASON_POINT_RENTALPLAYER")) return handleZiruMyLeagueExchangeRentalV71();
  if (contains(uri, "CMD_SET_MYCLUB_SPECIAL_DREAM_SQUAD_INFO")) return handleZiruMyLeagueSpecialSquadV71();
  if (contains(uri, "CMD_SET_SEASON_MISSION_DISPED")) return handleZiruLeagueStatic("responses/ZIRU_ML_SET_MISSION_DISPED.json");
  if (contains(uri, "CMD_SET_PLAYDATA_MATCH") && ziruMyLeagueIsActive()) return handleZiruLeagueStatic("responses/ZIRU_ML_SET_PLAYDATA_MATCH.json");
  if (contains(uri, "CMD_RECV_CAMPAIGN_PASS_REWARD") && ziruMyLeagueIsActive()) return handleZiruLeagueStatic("responses/ZIRU_ML_CAMPAIGN_PASS_REWARD.json");
  if (contains(uri, "CMD_GET_MYCLUB_PLAYER_DATA")) return handleZiruGetMyclubPlayerData(ziruLastRequestBody);
  if (contains(uri, "CMD_SET_MYCLUB_SQUAD_INFO")) return handleZiruSquadAck();
  if (contains(uri, "CMD_SELL_MYCLUB_GAMEPLAYER")) {
    auto x = handleZiruSellPlayer();
    if (!x) return newFixedText(500, "SERVER INTERNAL ERROR: Serve() returned a null response.");
    return *x;
  }
  if (contains(uri, "CMD_SET_TRACKRECORD")) return handleZiruTrackrecordMinimal();
  if (contains(uri, "CMD_SEND_ADJUST_PARAM")) return handleZiruAdjustParamMinimal();
  if (contains(uri, "CMD_SET_GAMERELAY_QUALITY")) return handleZiruGameRelayQualityMinimal();
  if (contains(uri, "CMD_IS_FIRST_LEAGUE")) {
    ziruMyLeagueSetActive(0);
    return handleZiruLeagueStatic("responses/ZIRU_IS_FIRST_LEAGUE.json");
  }
  if (contains(uri, "CMD_GET_LEAGUE_COM_INFO")) {
    logAlmanacAgentRequestBody(r);
    return handleZiruLeagueInfo();
  }
  if (contains(uri, "CMD_GET_GAMERESULT_LIST")) return handleZiruLeagueHistoryDynamic();
  if (contains(uri, "CMD_CHECK_MATCH_ENABLE"))
    return ziruMyLeagueIsActive() ? handleZiruLeagueStatic("responses/ZIRU_ML_CHECK_MATCH_ENABLE.json") : handleZiruLeagueStatic("responses/ZIRU_CHECK_MATCH_ENABLE.json");
  if (contains(uri, "CMD_GET_VSCOM_OPPONENT")) return ziruMyLeagueIsActive() ? handleZiruMyLeagueOpponentV72() : handleZiruLeagueOpponentDynamicV75();
  if (contains(uri, "CMD_START_GAME")) return ziruMyLeagueIsActive() ? handleZiruMyLeagueStartGameV71() : handleZiruLeagueStartGameDynamic();
  if (contains(uri, "CMD_GET_VSCOM_GAME_RESULT")) return ziruMyLeagueIsActive() ? handleZiruMyLeagueResultV71() : handleZiruLeagueResult();
  try {
    const char* a = routeAsset(uri);
    if (a) {
      std::string name = a;
      if (contains(uri, "COMMENTARY") && name == "get_myclub_commentary_info.json" && !contains(uri, "CmdGetMyclubCommentaryInfo.php")) isZiruLocalCommentaryInstalled();
      logRequest(name, "MATCH");
      return serveJsonAsEncryptedMsgPack("responses/" + name);
    }
    std::string sub = uri.substr(uri.rfind('/') == std::string::npos ? 0 : uri.rfind('/') + 1);
    if (startsWith(sub, "gate_")) sub = sub.substr(5);
    if (startsWith(sub, "GATE_")) sub = sub.substr(5);
    if (endsWith(sub, ".php")) {
      std::string rep = replaceAll(sub, ".php", ".json");
      try {
        Response x = serveJsonAsEncryptedMsgPack("responses/" + rep);
        logRequest(rep, "FALLBACK");
        return x;
      } catch (const IOEx&) {
        zs::logw("OFFLINE_SERVER", " [SERVER_MISS] No asset for fallback: " + rep);
      }
    }
    zs::logw("OFFLINE_SERVER", " [SERVER_NOT_FOUND] No match for: " + uri);
    logRequest(uri, "GENERIC_NO_RESPONSE");
    if (uri.find("/prd/banner/") != std::string::npos || uri.find(".png") != std::string::npos || uri.find(".jpg") != std::string::npos || uri.find(".webp") != std::string::npos) {
      return newFixed(404, "text/plain", {});
    }
    return handleZiruProtocolAckV81(std::nullopt);
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", " [SERVER_ERROR] Error handling " + uri + ": " + e.what());
    return newFixedText(500, e.what());
  }
}
