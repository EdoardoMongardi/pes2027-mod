// ---- part 5 (OfflineServer.java ~2805-3322) ----

// ---- Card identity ----
// What a card is printed with never changes: its two playing styles (0 = Basic),
// its AI playing styles (parameters 63..69), its club and nation, and the skills
// it has by itself. Only the exact card id counts: another version of the same
// player is a different card and is never used. Sources, first match wins:
//   1. the player records (ZIRU_PLAYER_RECORDS.json),
//   2. the starter squad (CmdLogin.json),
//   3. the MyLeague rental list (ZIRU_ML_RENTAL_LIST.json).
// A missing file is skipped, never an error.
struct ZiruCardIdentity {
  int style[2] = {0, 0};
  int ai[7] = {0, 0, 0, 0, 0, 0, 0};  // parameters 63..69
  bool hasClub = false, hasNation = false;
  int club = 0, nation = 0;
  bool hasSkills = false;             // false when the source has no skill flags
  std::set<int> skills;               // parameter indices 70..141 the card has by itself
};

std::unordered_map<int64_t, ObjP> ziruStarterCards, ziruRentalCards;
bool ziruStarterCardsLoaded = false, ziruRentalCardsLoaded = false;

// Exact-id index of the cards in one static list, built once. A file that cannot
// be read gives an empty index (logged once).
void z27IndexCards(const char* name, const char* listKey, const char* inner, bool& loaded,
                   std::unordered_map<int64_t, ObjP>& out) {
  if (loaded) return;
  loaded = true;
  try {
    ObjP root = Object::parse(readAsset(name));
    ObjP entry = root->optJSONObject("entry_info");
    ArrP list = entry ? entry->optJSONArray(listKey) : root->optJSONArray(listKey);
    for (int i = 0; list && i < list->length(); i++) {
      ObjP card = list->optJSONObject(i);
      if (card && inner) card = card->optJSONObject(inner);
      ObjP gid = card ? card->optJSONObject("gameplayer_id") : nullptr;
      int64_t id = gid ? gid->optLong("id", 0) : 0;
      if (id > 0 && !out.count(id)) out[id] = card;  // the same card listed twice: the first entry counts
    }
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[Z27_CARD_INDEX_ERR] ") + name + " " + e.what());
  }
}

// The card as its source prints it, or null when no source knows this exact id.
ObjP z27CardSource(int64_t id) {
  if (id <= 0) return nullptr;
  ObjP recs = ziruPlayerRecords();
  ObjP rec = recs ? recs->optJSONObject(str(id)) : nullptr;
  ObjP card = rec ? rec->optJSONObject("gameplayer") : nullptr;
  if (card) return card;
  z27IndexCards("responses/CmdLogin.json", "gameplayer_list", nullptr, ziruStarterCardsLoaded, ziruStarterCards);
  auto it = ziruStarterCards.find(id);
  if (it != ziruStarterCards.end()) return it->second;
  z27IndexCards("responses/ZIRU_ML_RENTAL_LIST.json", "rental_player_list", "player_info", ziruRentalCardsLoaded, ziruRentalCards);
  it = ziruRentalCards.find(id);
  return it != ziruRentalCards.end() ? it->second : nullptr;
}

bool z27CardIdentity(int64_t id, ZiruCardIdentity& out) {
  ObjP card = z27CardSource(id);
  if (!card) return false;
  ArrP styles = card->optJSONArray("play_style");
  ArrP values = card->optJSONArray("parameters");
  if (styles) {
    out.style[0] = styles->optInt(0, 0);
    out.style[1] = styles->optInt(1, 0);
  } else if (values && values->length() > 62) {
    out.style[0] = values->optInt(61, 0);
    out.style[1] = values->optInt(62, 0);
  }
  if (values && values->length() >= 70) {
    for (int i = 0; i < 7; ++i) out.ai[i] = values->optInt(63 + i, 0);
  }
  out.hasClub = card->has("initial_base_team_id");
  out.club = card->optInt("initial_base_team_id", 0);
  out.hasNation = card->has("initial_base_national_team_id");
  out.nation = card->optInt("initial_base_national_team_id", 0);
  if (values && values->length() >= 142) {
    out.hasSkills = true;
    for (int i = 70; i < 142; ++i) if (values->optInt(i, 0) == 1) out.skills.insert(i);
    // A starter entry already carries the skills its owner learned; those are not the card's.
    ArrP learned = card->optJSONArray("additional_skill_id");
    for (int i = 0; learned && i < learned->length(); ++i) {
      int skill = learned->optInt(i, -1);
      if (isZiruSkillId(skill)) out.skills.erase(kZiruSkillParameters[skill]);
    }
  }
  return true;
}

// Parameter indices of the skills a player has learned (additional_skill_id).
std::set<int> z27LearnedSkillParams(const ObjP& cur) {
  std::set<int> out;
  ArrP extra = cur ? cur->optJSONArray("additional_skill_id") : nullptr;
  for (int i = 0; extra && i < extra->length(); ++i) {
    int skill = extra->optInt(i, -1);
    if (isZiruSkillId(skill) && isZiruSkillParameter(kZiruSkillParameters[skill])) out.insert(kZiruSkillParameters[skill]);
  }
  return out;
}

// parameters[61]/[62] are the two playing styles. Keep them equal to play_style;
// an object without play_style gets it from those parameters.
void z27MirrorPlayStyle(const ObjP& cur) {
  if (!cur) return;
  ArrP styles = cur->optJSONArray("play_style");
  ArrP values = cur->optJSONArray("parameters");
  int first, second;
  if (styles) {
    first = styles->optInt(0, 0);
    second = styles->optInt(1, 0);
  } else if (values && values->length() > 62) {
    first = values->optInt(61, 0);
    second = values->optInt(62, 0);
  } else {
    return;
  }
  if (!styles || styles->length() != 2) cur->put("play_style", newArr()->put(first)->put(second));
  for (const char* key : {"parameters", "original_parameters"}) {
    ArrP arr = cur->optJSONArray(key);
    if (arr && arr->length() > 62) arr->put(61, first)->put(62, second);
  }
}

// Puts the card's identity on a player object. Learned skills stay on top of the
// card's own skills; any other skill flag is cleared. Returns false, and only
// mirrors the playing styles, when no source knows this exact card.
bool z27ApplyCardIdentity(const ObjP& cur) {
  ObjP gid = cur ? cur->optJSONObject("gameplayer_id") : nullptr;
  ZiruCardIdentity card;
  if (!gid || !z27CardIdentity(gid->optLong("id", 0), card)) {
    z27MirrorPlayStyle(cur);
    return false;
  }
  cur->put("play_style", newArr()->put(card.style[0])->put(card.style[1]));
  if (card.hasClub) cur->put("initial_base_team_id", card.club);
  if (card.hasNation) cur->put("initial_base_national_team_id", card.nation);
  std::set<int> learned = z27LearnedSkillParams(cur);
  for (const char* key : {"parameters", "original_parameters"}) {
    ArrP values = cur->optJSONArray(key);
    if (!values || values->length() < 70) continue;
    bool owned = std::string(key) == "parameters";
    values->put(61, card.style[0])->put(62, card.style[1]);
    for (int i = 0; i < 7; ++i) values->put(63 + i, card.ai[i]);
    if (!card.hasSkills || values->length() < 142) continue;
    for (int i = 70; i < 142; ++i) values->put(i, (card.skills.count(i) || (owned && learned.count(i))) ? 1 : 0);
  }
  return true;
}

// Learned skills as older builds saved them: Snap Strike (25) and Attack Trigger
// (68) stored as 72/73 are mapped back, Dummy skills and duplicates are dropped.
// A dropped skill's flag is cleared unless the card has it by itself.
void z27RepairLearnedSkills(const ObjP& cur) {
  ArrP extra = cur ? cur->optJSONArray("additional_skill_id") : nullptr;
  if (!extra) return;
  std::vector<int32_t> kept, dropped;
  for (int i = 0; i < extra->length(); ++i) {
    int stored = extra->optInt(i, -1);
    int skill = ziruRepairLearnedSkill(stored);
    if (skill >= 0 && !vcontains(kept, skill) && kept.size() < 5) kept.push_back(skill);
    else if (skill < 0 && isZiruSkillId(stored)) dropped.push_back(stored);
  }
  bool same = (int)kept.size() == extra->length();
  for (int i = 0; same && i < extra->length(); ++i) same = extra->optInt(i, -1) == kept[i];
  if (same) return;
  ArrP repaired = newArr();
  for (int skill : kept) repaired->put(skill);
  cur->put("additional_skill_id", repaired);
  ArrP values = cur->optJSONArray("parameters");
  ArrP printed = cur->optJSONArray("original_parameters");
  for (int skill : dropped) {
    int index = kZiruSkillParameters[skill];
    if (!values || !isZiruSkillParameter(index) || index >= values->length()) continue;
    if (!printed || printed->optInt(index, 0) != 1) values->put(index, 0);
  }
}

// Older builds wrote the flag of a learned 25/68 into parameters[143]/[142], which
// are booster slots in this layout, leaving booster id 1 there. No booster has id 1
// unless initial_booster_id_list says so; put the owned booster back.
void z27RepairBoosterSlots(const ObjP& cur) {
  ArrP boosters = cur ? cur->optJSONArray("initial_booster_id_list") : nullptr;
  for (const char* key : {"parameters", "original_parameters"}) {
    ArrP values = cur ? cur->optJSONArray(key) : nullptr;
    if (!values || values->length() < 145) continue;
    for (int b = 0; b < 3; ++b) {
      int owned = boosters ? boosters->optInt(b, 0) : 0;
      if (values->optInt(142 + b, 0) == 1 && owned != 1) values->put(142 + b, owned);
    }
  }
}

// ---- Last check before a response leaves the server ----
// The client applies a playing style in a match only when it is the only style
// on its side (attack or defence); the card still shows it otherwise. Side per
// pesdb value (Playstyle.bin): Box-to-Box (7) and Anchor Man (8) count on both
// sides; The Destroyer (9), Attacking/Defensive GK (16/17) and 23..33 are
// defence; every other style 1..35 is attack.
static int ziruStyleSide(int style) {
  if (style == 7 || style == 8) return 2;
  if (style == 9 || style == 16 || style == 17 || (style >= 23 && style <= 33)) return 1;
  return 0;
}
static bool ziruStylePairApplies(int first, int second) {
  if (first < 0 || first > 35 || second < 0 || second > 35) return false;
  if (first != 0 && first == second) return false;
  int attack = 0, defence = 0;
  for (int style : {first, second}) {
    if (style == 0) continue;
    int side = ziruStyleSide(style);
    if (side != 1) attack++;
    if (side != 0) defence++;
  }
  return attack <= 1 && defence <= 1;
}

std::set<int64_t> ziruStyleRuleLogged;  // cards already reported, so each is logged once

// One player object in an outgoing response.
void z27CheckOutgoingCard(const ObjP& o) {
  ObjP gid = o->optJSONObject("gameplayer_id");
  int64_t id = gid ? gid->optLong("id", 0) : 0;
  if (id <= 0) return;
  ArrP values = o->optJSONArray("parameters");
  int n = values ? values->length() : 0;
  bool cardParameters = n == 145 || n == 147 || n == 149;
  ArrP styles = o->optJSONArray("play_style");
  // Older list formats carry play_style as a single number, which the client reads as Basic/Basic.
  if (!cardParameters && !styles && !o->has("play_style")) return;
  ZiruCardIdentity card;
  if (z27CardIdentity(id, card)) {
    o->put("play_style", newArr()->put(card.style[0])->put(card.style[1]));
    for (const char* key : {"parameters", "original_parameters"}) {
      ArrP arr = o->optJSONArray(key);
      if (!arr || arr->length() < 70) continue;
      arr->put(61, card.style[0])->put(62, card.style[1]);
      for (int i = 0; i < 7; ++i) arr->put(63 + i, card.ai[i]);
    }
  } else {
    z27MirrorPlayStyle(o);
  }
  ArrP sent = o->optJSONArray("play_style");
  int first = sent ? sent->optInt(0, 0) : 0, second = sent ? sent->optInt(1, 0) : 0;
  if (!ziruStylePairApplies(first, second) && ziruStyleRuleLogged.insert(id).second) {
    std::string msg = "[Z27_STYLE_NOT_APPLIED] card " + str(id) + " play_style [" + str(first) + "," + str(second) +
                      "]: more than one style on one side, the match applies none there";
    zs::logw("OFFLINE_SERVER", msg);
    logRequest("Z27_STYLE_NOT_APPLIED", msg);
  }
}

// Every card object (gameplayer_id plus a parameter list, or plus any play_style),
// including every squad player_list entry, leaves with its own playing styles:
// from its identity source when one knows the exact card id, else
// parameters[61]/[62] follow play_style (or play_style is taken from them).
// Never throws.
void z27CheckOutgoingCards(Value& v, int depth = 0) {
  if (depth > 64) return;
  try {
    if (v.t == Value::ARR) {
      for (auto& item : v.a->v) z27CheckOutgoingCards(item, depth + 1);
    } else if (v.t == Value::OBJ) {
      z27CheckOutgoingCard(v.o);
      for (auto& kv : v.o->kv)
        if (kv.second.t == Value::OBJ || kv.second.t == Value::ARR) z27CheckOutgoingCards(kv.second, depth + 1);
    }
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[Z27_OUTGOING_CHECK_ERR] ") + e.what());
  }
}

void z27ApplyAppearanceAndClub(const ObjP& cur) {
  if (!cur) return;
  ObjP gid = cur->optJSONObject("gameplayer_id");
  if (!gid) return;
  int64_t pid = gid->optLong("id", 0);
  int32_t base_id = (int32_t)(pid & 0xFFFFFF);
  ArrP params = cur->optJSONArray("parameters");

  // 1. Playing styles, AI playing styles, club, nation and the card's own skills
  //    come from the card itself (exact id), never from a guess.
  z27RepairLearnedSkills(cur);
  z27RepairBoosterSlots(cur);
  z27ApplyCardIdentity(cur);

  // 2. Fix skin tone parameter 49 if 0 (chalk-white albino bug)
  auto restoreAppearance = [&](const ArrP& values) {
    if (!values || values->length() < 61) return;
    if (base_id == 133543 || base_id == 133580 || base_id == 40352) {
      // Licensed models (Neymar, Haaland): parameters 49..60 must remain 0 so native 3D face/braids load properly
      for (int i = 0; i < 12; ++i) values->put(49 + i, 0);
      return;
    }
    bool missing = true;
    bool corrupt = false;
    for (int i = 49; i <= 60; ++i) if (values->optInt(i, 0) == 99) corrupt = true;
    for (int i = 50; i <= 60; ++i) if (values->optInt(i, 0) != 0) missing = false;
    if (corrupt) missing = true;
    auto preset = kAppearancePresets.find(base_id);
    bool presetValid = false;
    if (preset != kAppearancePresets.end()) {
      for (int i = 0; i < 12; ++i) if (preset->second.v[i] != 0 && preset->second.v[i] != 99) presetValid = true;
    }
    if (missing && presetValid) {
      for (int i = 0; i < 12; ++i) values->put(49 + i, preset->second.v[i]);
    } else if (missing) {
      static const int defFace[12] = {3, 1, 1, 2, 3, 0, 2, 0, 2, 2, 48, 0};
      for (int i = 0; i < 12; ++i) values->put(49 + i, defFace[i]);
    }
  };
  restoreAppearance(params);
  if (params && params->length() >= 65) {
    if (base_id != 133543 && base_id != 133580 && base_id != 40352 && params->optInt(49, 0) <= 0) params->put(49, 3);
  }

  ArrP origParams = cur->optJSONArray("original_parameters");
  restoreAppearance(origParams);
  if (origParams && origParams->length() >= 70) {
    if (base_id != 133543 && origParams->optInt(49, 0) <= 0) origParams->put(49, 3);
  }
}

// One normalisation for a user's card wherever it is sent or played (login
// roster, entry info, player detail, careerplan, match start), so the card the
// client shows is the card the match uses.
void z27NormalizeOwnedCard(const ObjP& p) {
  if (!p) return;
  z27ApplyAppearanceAndClub(p);
  applyZiruRonaldoMaxProgression(p);
  applyZiruWeakFootFix(p);
  ziruSyncActiveBooster(p);
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
        // Only this exact card's record counts; another version of the player is another card.
        ObjP w = getZiruPlayerWrapper(Value((long long)id), false);
        ObjP base = w ? w->optJSONObject("gameplayer") : nullptr;
        if (base) {
          ArrP ib = base->optJSONArray("initial_booster_id_list");
          ArrP curIb = cur->optJSONArray("initial_booster_id_list");
          if (!curIb) {
            curIb = newArr()->put(0)->put(0)->put(0);
            cur->put("initial_booster_id_list", curIb);
          }
          while (curIb->length() < 3) curIb->put(0);
          if (ib) {
            for (int b = 0; b < ib->length() && b < 3; ++b) {
              int bidVal = ib->optInt(b, 0);
              if (bidVal > 0 && curIb->optInt(b, 0) == 0) curIb->put(b, bidVal);
            }
          }
          // Playing styles, AI styles and the card's own skills are set by
          // z27ApplyAppearanceAndClub below, from this same record.
          ArrP baseParams = base->optJSONArray("parameters");
          ArrP curParams = cur->optJSONArray("parameters");
          if (baseParams && curParams) {
            while (curParams->length() < 145) curParams->put(0);
            for (int b = 0; b < 3; ++b) {
              int bVal = curIb->optInt(b, 0);
              if (bVal > 0 && curParams->optInt(142 + b, 0) == 0) curParams->put(142 + b, bVal);
              else if (baseParams->length() >= 145 && curParams->optInt(142 + b, 0) == 0) {
                int bpVal = baseParams->optInt(142 + b, 0);
                if (bpVal > 0) {
                  curParams->put(142 + b, bpVal);
                  if (curIb->optInt(b, 0) == 0) curIb->put(b, bpVal);
                }
              }
            }
          }
          ArrP origParams = cur->optJSONArray("original_parameters");
          if (baseParams && origParams) {
            while (origParams->length() < 145) origParams->put(0);
            for (int b = 0; b < 3; ++b) {
              int bVal = curIb->optInt(b, 0);
              if (bVal > 0 && origParams->optInt(142 + b, 0) == 0) origParams->put(142 + b, bVal);
            }
          }
        }
        // Level limit: from the card's record, else the card's own level_info.
        ObjP bl = base ? base->optJSONObject("level_info") : cur->optJSONObject("level_info");
        if (base || bl) {
          int lim = bl ? bl->optInt("level_limit", 1) : 1;
          if (lim == 1 && (((int32_t)id) & 16777215) == 134665) {
            lim = 33;
            logRequest("ZIRU_V16_LEVEL", "[ZIRU_V16_ADRIANO_FALLBACK] base_id=134665 level_limit=33");
          }
          if ((((int32_t)id) & 16777215) == 4522 || id == 89138556572074LL) {
            lim = 36;
            logRequest("ZIRU_CR7_UNLOCK", "[ZIRU_CR7_UNNERFED] Unlocked Prime CR7 level limit to 36");
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
        std::vector<int32_t> al;
        std::string s = sp("almanac_state").getString("ziru_skills_" + str(serial), "");
        if (!s.empty()) {
          for (auto& part : split(s, ',')) {
            try {
              int v = ziruRepairLearnedSkill(parseInt(part));
              if (isZiruSkillId(v) && !vcontains(al, v) && al.size() < 5) al.push_back(v);
            } catch (...) {}
          }
        }
        if (al.empty()) {
          ArrP addSkills = cur->optJSONArray("additional_skill_id");
          if (addSkills) {
            for (int i = 0; i < addSkills->length(); i++) {
              int v = ziruRepairLearnedSkill(addSkills->optInt(i, -1));

              if (isZiruSkillId(v) && !vcontains(al, v) && al.size() < 5) al.push_back(v);
            }
          }
        }
        ArrP ja = newArr();
        for (int v : al) ja->put(v);
        cur->put("additional_skill_id", ja);
        ArrP params = cur->optJSONArray("parameters");
        if (params && params->length() >= 70) {
          for (int v : al) {
            if (isZiruSkillId(v)) {
              int pIdx = kZiruSkillParameters[v];
              if (isZiruSkillParameter(pIdx) && pIdx < params->length()) params->put(pIdx, 1);
            }
          }
        }
        z27NormalizeOwnedCard(cur);
        if (serial > 0) {
          ziruDb->savePlayer(serial, id, cur->toString());
        }
        return cur;
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
    auto boostCoach = [](const ObjP& c) -> ObjP {
      ObjP res = Object::parse(c->toString());
      res->put("possession", 99);
      res->put("short_counter", 99);
      res->put("long_ball", 99);
      res->put("long_counter", 99);
      res->put("side_attack", 99);
      res->put("overload", 99);
      return res;
    };
    for (int i = 0; i < cl->length(); i++) {
      ObjP c = cl->optJSONObject(i);
      ObjP cid;
      if (c && (cid = c->optJSONObject("coach_id")) && cid->optLong("id", 0) == id) return boostCoach(c);
    }
    if (cl->length() > 0) return boostCoach(cl->getJSONObject(0));
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

// Saves from builds before card identity keep player copies outside the players
// table. Correct them once: the learned-skill lists in prefs, the recruited
// players list, and the match squad cached for the active division match (it is
// rebuilt at the next match start). The players table itself is corrected and
// re-saved by the normal login path.
void z27MigrateSavedCards() {
  if (loadMeta("z27_card_identity_v1")) return;
  try {
    zs::Prefs& prefs = sp("almanac_state");
    auto edit = prefs.edit();
    bool changed = false;
    for (const auto& key : prefs.data->keys()) {
      if (!startsWith(key, "ziru_skills_")) continue;
      std::string stored = prefs.getString(key, "");
      std::vector<int32_t> kept;
      for (auto& part : split(stored, ',')) {
        try {
          int skill = ziruRepairLearnedSkill(parseInt(part));
          if (skill >= 0 && !vcontains(kept, skill) && kept.size() < 5) kept.push_back(skill);
        } catch (const std::exception&) {}
      }
      std::string repaired = joinInts(kept);
      if (repaired != stored) {
        if (kept.empty()) edit.remove(key);
        else edit.putString(key, repaired);
        changed = true;
      }
    }
    std::string recruited = prefs.getString("recruited_players", "");
    if (!recruited.empty()) {
      ArrP list = Array::parse(recruited);
      for (int i = 0; i < list->length(); i++) {
        ObjP o = list->optJSONObject(i);
        ObjP gp = o ? o->optJSONObject("gameplayer") : nullptr;
        z27ApplyAppearanceAndClub(gp ? gp : o);
      }
      std::string repaired = list->toString();
      if (repaired != recruited) {
        edit.putString("recruited_players", repaired);
        changed = true;
      }
    }
    if (changed) edit.commit();
    ObjP active = z27Meta("z27_active_match");
    if (active && active->has("home_squad")) {
      active->remove("home_squad");
      z27PutMeta("z27_active_match", active->toString());
    }
    ziruDb->saveMeta("z27_card_identity_v1", "1");
    logRequest("Z27_CARD_MIGRATION", "[Z27_CARD_MIGRATION] saved card copies corrected");
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[Z27_CARD_MIGRATION_ERR] ") + e.what());
  }
}
