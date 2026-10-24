// ---- MyClub: recruit / training / skills (OfflineServer.java ~105-800) ----

int32_t z28RandomBooster(bool isGk = false) {
  ArrP a = nn(loadZiruJsonAssetObject("responses/ZIRU_RANDOM_BOOSTERS.json"))->getJSONArray("ability_ids");
  static const std::set<int> gkBoosters = {22, 33, 44, 73, 79, 85, 271, 451, 495, 502};
  zc::JavaRandom r;
  int attempts = 0;
  while (attempts++ < 100) {
    int32_t b = a->getInt(r.nextInt(a->length()));
    bool isGkBooster = (gkBoosters.find(b) != gkBoosters.end());
    if (isGk && isGkBooster) return b;
    if (!isGk && !isGkBooster) return b;
  }
  return isGk ? 22 : 12; // fallback GK Saving vs Outfield Agility
}

// Booster training on `target`, the card the client picked (z27TrainingTarget).
void applyZiruBoosterTrainingResponse(const ObjP& r13, const ObjP& target) {
  try {
    if (!ziruItemType) return;
    std::string r0 = *ziruItemType;
    int32_t r3 = ziruTrainingIndex;
    if (!(r3 >= 0 && r3 < 3)) r3 = 1;
    int64_t r4 = ziruTargetSerial;
    ObjP r8 = ziruSyncActiveBooster(target);
    ArrP r9 = nn(r8)->optJSONArray("parameters");
    bool isGk = (r9 && r9->length() > 0 && r9->optInt(0, -1) == 0);

    int32_t r2;
    if (r0 == "ABILITY_TRAINING_RANDOM") {
      r2 = z28RandomBooster(isGk);
    } else if (r0 == "ABILITY_TRAINING_SELECT") {
      r2 = ziruAbilityId;
      if (r2 <= 0) { logRequest("ZIRU_BOOSTER_V23", "[ZIRU_BOOSTER_V23_FAIL] missing target/ability"); return; }
    } else {
      return;
    }
    if (r9 && r9->length() >= 145) r9->put(142 + r3, r2);
    ArrP ib = r8->optJSONArray("initial_booster_id_list");
    if (!ib) { ib = newArr()->put(0)->put(0)->put(0); r8->put("initial_booster_id_list", ib); }
    while (ib->length() < 3) ib->put(0);
    ib->put(r3, r2);
    saveZiruPlayerToDb(r8);
    ObjP res = r13->optJSONObject("ability_training_result");
    if (!res) { res = newObj(); r13->put("ability_training_result", res); }
    res->put("ability_Id", r2);
    res->put("ability_id", r2);
    res->put("is_success", "YES");
    logRequest("ZIRU_BOOSTER_V23", "[ZIRU_BOOSTER_V23] type=" + r0 + " serial=" + str(r4) + " slot=" + str(r3) + " ability_id=" + str(r2));
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_BOOSTER_V23_ERR] ") + e.what());
    throw;
  }
}

void applyZiruDynamicRecruit(const ObjP& jo) {
  try {
    int64_t j = ziruLastAgentId;
    if (j <= 0) return;
    int32_t i = ziruLastCount;
    if (i <= 0) i = 1;
    ArrP packs = Object::parse(readAsset("responses/ZIRU_PACKS_CONFIG.json"))->getJSONArray("packs");
    ObjP pack;
    for (int i2 = 0; i2 < packs->length(); i2++) {
      ObjP p = packs->optJSONObject(i2);
      if (p && p->optLong("agent_id") == j) { pack = p; break; }
    }
    if (!pack) { zs::logi("OFFLINE_SERVER", "[ZIRU_V10_DRAW] agent_id not in ZIRU config; leaving template unchanged"); return; }
    ArrP ids = pack->getJSONArray("player_ids");
    zs::Prefs& prefs = sp("almanac_state");
    std::string key = "ziru_remaining_" + str(j);
    std::string s = prefs.getString(key, "");
    ArrP remaining = s.empty() ? Array::parse(ids->toString()) : Array::parse(s);
    int length = remaining->length();
    if (length <= 0) {
      remaining = Array::parse(ids->toString());
      length = remaining->length();
      zs::logi("OFFLINE_SERVER", "[ZIRU_V10_DRAW] pack auto-reset on exhaustion");
    }
    if (i > length) i = length;
    ArrP list = jo->optJSONArray("gameplayer_list");
    ObjP o1, o2, o3;
    if (!list || list->length() <= 0 || !(o1 = list->optJSONObject(0)) || !(o2 = o1->optJSONObject("gameplayer")) ||
        !(o3 = o2->optJSONObject("gameplayer_id")))
      return;
    int64_t j2 = prefs.getLong("ziru_next_serial", o3->optLong("serial"));
    ArrP out = newArr();
    zc::JavaRandom random;
    for (int i3 = 0; i3 < i; i3++) {
      Value removed = remaining->remove(random.nextInt(remaining->length()));
      ObjP w = getZiruPlayerWrapper(removed, true);
      if (!w) {
        w = Object::parse(o1->toString());
        w->getJSONObject("gameplayer")->getJSONObject("gameplayer_id")->put("id", removed);
      }
      j2++;
      w->getJSONObject("gameplayer")->getJSONObject("gameplayer_id")->put("serial", (long long)j2);
      refreshZiruGameplayerFromCanonical(w->getJSONObject("gameplayer"));
      saveZiruPlayerToDb(w->getJSONObject("gameplayer"));
      out->put(w);
    }
    jo->put("gameplayer_list", out);
    prefs.edit().putString(key, remaining->toString()).putLong("ziru_next_serial", j2).commit();
    std::string msg = "[ZIRU_V10_DRAW] count=" + str(i) + " remaining=" + str(remaining->length());
    zs::logi("OFFLINE_SERVER", msg);
    logRequest("ZIRU_V10_DRAW", msg);
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V10_DRAW_ERR] dynamic recruit failed ") + e.what());
    throw;
  }
}

static inline int z27GetPositionIndexByName(const std::string& name) {
  static const char* const names[] = {
    "", "CF", "ST", "LWG", "RWG", "OMF", "DMF", "CMF", "LMF", "RMF", "CB", "LSB", "RSB"
  };
  for (int i = 1; i <= 12; i++) {
    if (name == names[i]) return i;
  }
  if (name == "SS") return 2;
  if (name == "AMF") return 5;
  if (name == "LB") return 11;
  if (name == "RB") return 12;
  return -1;
}

// parameters[1..13] index of a Position number (GK 0, CB 1, LB 2, RB 3, DMF 4, CMF 5, LMF 6,
// RMF 7, AMF 8, LWF 9, RWF 10, SS 11, CF 12), as iOS tmpdb::Player::GetDataLabel(Position)
// maps it (labelTable 0x10578ba08); -1 for anything else.
static inline int z27PositionParameter(int position) {
  static const int index[13] = {13, 10, 11, 12, 6, 7, 8, 9, 5, 3, 4, 2, 1};
  return position >= 0 && position < 13 ? index[position] : -1;
}

// Position training items are never used up.
static void z27KeepPositionTrainingItems(const ObjP& jo) {
  ArrP items = jo->optJSONArray("item_list");
  for (int i = 0; items && i < items->length(); i++) {
    ObjP it = items->optJSONObject(i);
    ObjP item;
    if (it && (item = it->optJSONObject("item"))) {
      std::string t = item->optString("item_type", "");
      if (t == "POSITION_TRAINING" || t == "POSITION_TRAINING_SELECT") it->put("inventory_num", 999999);
    }
  }
}

// Position training on `target`, the card the client picked (z27TrainingTarget).
void applyZiruPositionTrainingResponse(const ObjP& jo, const ObjP& target) {
  try {
    if (!(ziruItemType && (*ziruItemType == "POSITION_TRAINING" || *ziruItemType == "POSITION_TRAINING_SELECT"))) return;
    int64_t j = ziruTargetSerial;
    if (j > 0) {
      ObjP p = target;
      int64_t cardId = p->getJSONObject("gameplayer_id")->optLong("id", ziruTargetPlayerId);
      ArrP params = p->optJSONArray("parameters");
      if (params && params->length() >= 14) {
        if (params->optInt(0) != 0) {
          static const char* const names[] = {
            "", "CF", "ST", "LWG", "RWG", "OMF", "DMF", "CMF", "LMF", "RMF", "CB", "LSB", "RSB"
          };
          ArrP add = p->optJSONArray("additional_position");
          if (!add) { add = newArr(); p->put("additional_position", add); }

          int slot = ziruTrainingIndex;
          if (slot != 0 && slot != 1) {
            slot = (add->length() == 0) ? 0 : 1;
          }
          if (add->length() == 0) {
            slot = 0;
          }

          // The card's own record, never another card's.
          ObjP baseWrapper = getZiruPlayerWrapper(Value((long long)cardId), false);
          ObjP baseP;
          ArrP baseParams;
          if (baseWrapper && (baseP = baseWrapper->optJSONObject("gameplayer"))) {
            baseParams = baseP->optJSONArray("parameters");
          }

          int other_slot = (slot == 0) ? 1 : 0;
          std::string other_pos = (other_slot < add->length()) ? add->optString(other_slot, "") : "";
          int other_pos_idx = z27GetPositionIndexByName(other_pos);

          // Position Training (Select) teaches the position the client picked, when the card can
          // learn it: an outfield position the card does not play by itself and that the other slot
          // does not hold. Anything else is answered as not successful; nothing changes and the
          // item is not used up.
          bool select = *ziruItemType == "POSITION_TRAINING_SELECT";
          if (select) {
            int k = ziruPickedPosition;
            std::string old_pos = (slot < add->length()) ? add->optString(slot, "") : "";
            bool hasBase = baseParams && baseParams->length() >= 14;
            int level = hasBase ? baseParams->optInt(k) : (k == z27GetPositionIndexByName(old_pos) ? 0 : params->optInt(k));
            if (!(k >= 1 && k <= 12 && k != other_pos_idx && level < 2)) {
              for (const char* other : {"skill_training_result", "ability_training_result"}) jo->remove(other);
              jo->put("position_training_result", newObj()->put("additional_position", "")->put("is_success", "NO"));
              z27KeepPositionTrainingItems(jo);
              logRequest("ZIRU_POSITION_V26", "[ZIRU_POSITION_SELECT_REFUSED] serial=" + str(j) + " slot=" + str(slot) +
                                                  " position=" + str(k) + " cannot be learned");
              return;
            }
          }

          if (baseParams && baseParams->length() >= 14) {
            for (int k = 1; k <= 12; k++) {
              params->put(k, baseParams->optInt(k));
            }
          } else {
            std::string old_pos = (slot < add->length()) ? add->optString(slot, "") : "";
            int old_idx = z27GetPositionIndexByName(old_pos);
            if (old_idx >= 1 && old_idx <= 12) {
              params->put(old_idx, 0);
            }
          }

          if (other_pos_idx >= 1 && other_pos_idx <= 12) {
            params->put(other_pos_idx, 2);
          }

          std::vector<int> candidates;
          for (int k = 1; k <= 12; k++) {
            if (k == other_pos_idx) continue;
            if (baseParams && baseParams->length() >= 14 && baseParams->optInt(k) >= 2) continue;
            if ((!baseParams || baseParams->length() < 14) && params->optInt(k) >= 2) continue;
            candidates.push_back(k);
          }

          if (candidates.empty()) {
            for (int k = 1; k <= 12; k++) {
              if (k != other_pos_idx) candidates.push_back(k);
            }
          }

          int chosen_idx = select ? ziruPickedPosition : candidates[rand() % candidates.size()];
          std::string chosen_pos = names[chosen_idx];

          params->put(chosen_idx, 2);

          if (slot == 0) {
            if (add->length() == 0) add->put(chosen_pos);
            else add->put(0, chosen_pos);
          } else {
            if (add->length() == 0) {
              add->put(chosen_pos);
            } else if (add->length() == 1) {
              add->put(chosen_pos);
            } else {
              add->put(1, chosen_pos);
            }
          }

          saveZiruPlayerToDb(p);

          ObjP res = jo->optJSONObject("position_training_result");
          if (!res) { res = newObj(); jo->put("position_training_result", res); }
          res->put("additional_position", chosen_pos);
          res->put("is_success", "YES");

          z27KeepPositionTrainingItems(jo);

          logRequest("ZIRU_POSITION_V26", "[ZIRU_POSITION_V26] type=" + *ziruItemType + " serial=" + str(j) + " slot=" + str(slot) +
                                              " position=" + chosen_pos);
          return;
        }

        // A goalkeeper learns no other position.
        for (const char* other : {"skill_training_result", "ability_training_result"}) jo->remove(other);
        jo->put("position_training_result", newObj()->put("additional_position", "")->put("is_success", "NO"));
        z27KeepPositionTrainingItems(jo);
        return;
      }
    }
    logRequest("ZIRU_POSITION_V26", "[ZIRU_POSITION_V26_FAIL] missing/invalid player state");
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_POSITION_V26_ERR] ") + e.what());
  }
}

void applyZiruProcurableList(const ObjP& jo) {
  try {
    int32_t i = (int32_t)ziruLastAgentId;
    if (i <= 0) return;
    ArrP packs = Object::parse(readAsset("responses/ZIRU_PACKS_CONFIG.json"))->getJSONArray("packs");
    ObjP pack;
    for (int i2 = 0; i2 < packs->length(); i2++) {
      pack = packs->optJSONObject(i2);
      if (pack && pack->optInt("agent_id") == i) break;
      pack = nullptr;
    }
    if (!pack) return;
    ArrP ids = pack->getJSONArray("player_ids");
    std::string s = sp("almanac_state").getString("ziru_remaining_" + str(i), "");
    if (!s.empty()) ids = Array::parse(s);
    ArrP list = jo->optJSONArray("gameplayer_list");
    ObjP first;
    if (!list || list->length() <= 0 || !(first = list->optJSONObject(0))) return;
    ArrP out = newArr();
    for (int i3 = 0; i3 < ids->length(); i3++) {
      Value opt = ids->opt(i3);
      ObjP w = getZiruPlayerWrapper(opt, false);
      if (w) out->put(w);
      else {
        ObjP c = Object::parse(first->toString());
        c->getJSONObject("gameplayer")->getJSONObject("gameplayer_id")->put("id", opt);
        out->put(c);
      }
    }
    jo->put("gameplayer_list", out);
    jo->put("acquired_player_list", newArr());
    zs::logi("OFFLINE_SERVER", "[ZIRU_V11_PROC] agent_id=" + str(i) + " players=" + str(out->length()));
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V11_PROC_ERR] failed ") + e.what());
  }
}

void applyZiruProgressionToCareerList(const ArrP& ja, int64_t j) {
  try {
    JStr lp = loadProgression(j);
    if (!lp || lp->empty()) return;
    ObjP prog = Object::parse(*lp);
    ObjP first = nn(ja)->optJSONObject(0);
    ArrP dl;
    if (!first || !(dl = first->optJSONArray("data_list"))) return;
    for (int i = 0; i < dl->length(); i++) {
      ObjP d = dl->optJSONObject(i);
      if (d) {
        int c = ziruCountFromSpent(prog->optInt(d->optString("parameter_type", ""), 0));
        d->put("count", c);
        d->put("point", (c / 4) + 1);
      }
    }
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V14_PROGRESSION_LIST_ERR] ") + e.what());
  }
}

void applyZiruRemainingCounts(const ObjP& jo) {
  try {
    ArrP list = jo->optJSONArray("agent_list");
    if (!list) return;
    zs::Prefs& prefs = sp("almanac_state");
    for (int i = 0; i < list->length(); i++) {
      ObjP a = list->optJSONObject(i);
      if (!a) continue;
      int64_t id = a->optLong("agent_id");
      std::string s = prefs.getString("ziru_remaining_" + str(id), "");
      if (!s.empty()) {
        int len = Array::parse(s)->length();
        ArrP c = newArr();
        c->put(0); c->put(0); c->put(len); c->put(0); c->put(0);
        a->put("remaining_player_count", c);
        logRequest("ZIRU_V12_2_REMAINING", "[ZIRU_V12_2_REMAINING] agent_id=" + str(id) + " remaining=" + str(len));
      }
    }
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V12_2_REMAINING_ERR] failed ") + e.what());
  }
}

void applyZiruResetBox(const ObjP& jo) {
  try {
    int64_t j = ziruLastAgentId;
    if (j > 0) {
      sp("almanac_state").edit().remove("ziru_remaining_" + str(j)).commit();
      jo->put("agent_id", (long long)j);
      ziruAttachResetCounts(jo, j);
      logRequest("ZIRU_V14_RESET", "[ZIRU_V14_RESET] agent_id=" + str(j) + " restored");
    }
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V14_RESET_ERR] failed ") + e.what());
  }
}

void applyZiruRonaldoMaxProgression(const ObjP& jo) {
  if (!jo) return;
  try {
    ObjP gid = jo->optJSONObject("gameplayer_id");
    if (!gid) return;
    int64_t pid = gid->optLong("id", 0);
    int32_t base_id = (int32_t)(pid & 0xFFFFFF);
    if (base_id != 4522 && pid != 89138556572074LL) return;
    ArrP p = jo->optJSONArray("parameters");
    if (p && p->length() >= 41) {
      // Prime Cristiano Ronaldo God-Tier Base Stats (Unlocked from Ziru nerf)
      p->put(14, 96); // Offensive Awareness
      p->put(17, 92); // Ball Control
      p->put(18, 92); // Dribbling
      p->put(19, 90); // Tight Possession
      p->put(20, 97); // Finishing
      p->put(23, 95); // Heading
      p->put(27, 88); // Place Kicking
      p->put(28, 86); // Curl
      p->put(34, 99); // Kicking Power
      p->put(35, 96); // Speed
      p->put(36, 93); // Acceleration
      p->put(37, 93); // Physical Contact
      p->put(38, 99); // Jump (Iconic CR7 Leap)
      p->put(40, 92); // Stamina
    }
    ObjP li = jo->optJSONObject("level_info");
    if (li) {
      int lim = std::max(36, li->optInt("level_limit", 36));
      li->put("level_limit", lim);
      li->put("level", lim);
      int cp = ziruProgressionBudget(lim);
      li->put("customize_point", cp);
      li->put("exp_current_level", 0);
      li->put("exp_to_next_level", 0);
    }
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_RONALDO_UNLOCKED_ERR] ") + e.what());
  }
}

inline const std::unordered_map<int32_t, std::pair<int, int>>& getZiruCanonicalWeakFootMap() {
  static const std::unordered_map<int32_t, std::pair<int, int>> kMap = {
    {52, {3, 3}},
    {812, {3, 2}},    // Ruud Gullit legend base
    {852, {1, 0}},
    {866, {1, 0}},
    {867, {2, 3}},    // Kaka
    {3758, {2, 2}},
    {4073, {1, 1}},
    {4522, {3, 2}},   // Cristiano Ronaldo
    {7444, {1, 0}},
    {7511, {2, 0}},   // Lionel Messi
    {33185, {2, 1}},
    {33702, {2, 2}},
    {34881, {3, 2}},
    {40240, {3, 3}},  // Neymar Jr
    {40323, {2, 1}},
    {40352, {3, 2}},  // Luis Suárez
    {41126, {2, 2}},
    {44379, {3, 2}},  // Kevin De Bruyne
    {45097, {3, 3}},  // Son Heung-Min
    {45993, {1, 1}},
    {57123, {1, 1}},
    {104677, {2, 2}},
    {104717, {1, 1}},
    {108959, {2, 1}},
    {109941, {1, 1}},
    {110718, {2, 2}},  // Ousmane Dembélé
    {113960, {2, 2}},
    {114105, {2, 2}},
    {115881, {2, 0}},
    {116543, {2, 1}},
    {129369, {1, 0}},
    {132933, {3, 2}},
    {133543, {2, 0}},  // Erling Haaland
    {134577, {1, 0}},
    {134589, {2, 1}},
    {134594, {2, 2}},
    {134595, {2, 2}},
    {134604, {1, 0}},
    {134605, {2, 1}},
    {134608, {2, 1}},
    {134634, {3, 2}},  // Ruud Gullit (Epic / Big Time)
    {134635, {3, 3}},
    {134637, {3, 2}},
    {134639, {2, 1}},
    {137962, {3, 2}},
    {137990, {2, 2}},
    {138000, {2, 0}},
    {138392, {1, 1}},
    {139679, {2, 2}},
    {160577, {1, 1}},
    {166176, {3, 3}}
  };
  return kMap;
}

void applyZiruWeakFootFix(const ObjP& jo) {
  if (!jo) return;
  try {
    ObjP gid = jo->optJSONObject("gameplayer_id");
    if (!gid) return;
    int64_t pid = gid->optLong("id", 0);
    int32_t base_id = (int32_t)(pid & 0xFFFFFF);
    ArrP p = jo->optJSONArray("parameters");
    if (!p || p->length() < 46) return;

    if (pid == 88041460993514LL || base_id == 134634 || base_id == 812) {
      p->put(33, 3); // Weak Foot Accuracy: Very High
      p->put(45, 2); // Weak Foot Usage: Frequently
      return;
    }
    if (base_id == 40352) { // Luis Suárez
      p->put(33, 3); // Weak Foot Accuracy: Very High
      p->put(45, 2); // Weak Foot Usage: Frequently
      return;
    }

    const auto& map = getZiruCanonicalWeakFootMap();
    auto it = map.find(base_id);
    if (it != map.end()) {
      p->put(33, it->second.first);
      p->put(45, it->second.second);
      return;
    }

    if (p->optInt(33, 0) <= 0) {
      int finish = p->optInt(20, 50);
      int pass = p->optInt(21, 50);
      int dribble = p->optInt(18, 50);
      int maxAtt = std::max({finish, pass, dribble});
      if (maxAtt >= 85) {
        p->put(33, 3);
      } else if (maxAtt >= 75) {
        p->put(33, 2);
      } else {
        p->put(33, 1);
      }
      if (p->optInt(45, 0) <= 0 && maxAtt >= 75) {
        p->put(45, 1);
      }
    }
  } catch (...) {}
}

static bool vcontains(const std::vector<int32_t>& v, int32_t x) { return std::find(v.begin(), v.end(), x) != v.end(); }

void applyZiruSkillStateToLogin(const ObjP& jo) {
  try {
    ObjP ei = jo->optJSONObject("entry_info");
    ArrP list;
    if (!ei || !(list = ei->optJSONArray("gameplayer_list"))) return;
    zs::Prefs& prefs = sp("almanac_state");
    for (int i = 0; i < list->length(); i++) {
      ObjP p = list->optJSONObject(i);
      ObjP gid;
      if (!p || !(gid = p->optJSONObject("gameplayer_id"))) continue;
      int64_t serial = gid->optLong("serial", 0);
      if (serial == 0) continue;
      int64_t pid = gid->optLong("id", 0);
      // The learned skills and the row under this serial belong to the card saved
      // there; a listed card that is another card keeps only its own skills.
      JStr saved = loadPlayer(serial);
      bool serialState = !saved || saved->empty();
      if (!serialState) {
        ObjP row = Object::parse(*saved);
        if (row->optJSONObject("gameplayer")) row = row->getJSONObject("gameplayer");
        serialState = z27IsCard(row, pid);
      }
      std::vector<int32_t> al;
      std::string s = serialState ? prefs.getString("ziru_skills_" + str(serial), "") : std::string();
      if (!s.empty()) {
        for (auto& part : split(s, ',')) {
          try {
            int v = ziruRepairLearnedSkill(parseInt(part));
            if (isZiruSkillId(v) && !vcontains(al, v) && al.size() < 5) al.push_back(v);
          } catch (const NumberFormat&) {
          }
        }
      }
      if (al.empty()) {
        ArrP sk = p->optJSONArray("additional_skill_id");
        if (sk) {
          for (int i2 = 0; i2 < sk->length(); i2++) {
            int v = ziruRepairLearnedSkill(sk->optInt(i2));

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
            if (isZiruSkillParameter(pIdx) && pIdx < params->length()) {
              params->put(pIdx, 1);
            }
          }
        }
      }
      // The card's own skills plus the learned ones (exact card only).
      z27ApplyCardIdentity(p);
      if (!serialState) continue;
      if (!al.empty() && prefs.getString("ziru_skills_" + str(serial), "") != joinInts(al)) {
        prefs.edit().putString("ziru_skills_" + str(serial), joinInts(al)).commit();
      }
      if (!saved || *saved != p->toString()) saveZiruPlayerToDb(p);
    }
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_SKILL_V2] login merge/reapply failed ") + e.what());
    throw;
  }
}

static std::string joinInts(const std::vector<int32_t>& v) {
  std::string s;
  for (size_t i = 0; i < v.size(); i++) { if (i) s += ","; s += std::to_string(v[i]); }
  return s;
}

// Parameter indices of the skills a card has by itself: from its identity
// source, else (unknown card) its original_parameters, else its parameters
// without the learned skills.
std::set<int> z27CardSkillParams(const ObjP& p) {
  ObjP gid = p ? p->optJSONObject("gameplayer_id") : nullptr;
  ZiruCardIdentity card;
  if (gid && z27CardIdentity(gid->optLong("id", 0), card) && card.hasSkills) return card.skills;
  std::set<int> out;
  ArrP printed = p ? p->optJSONArray("original_parameters") : nullptr;
  if (printed && printed->length() >= 142) {
    for (int i = 70; i < 142; ++i) if (printed->optInt(i, 0) == 1) out.insert(i);
    return out;
  }
  ArrP values = p ? p->optJSONArray("parameters") : nullptr;
  std::set<int> learned = z27LearnedSkillParams(p);
  for (int i = 70; values && i < 142 && i < values->length(); ++i)
    if (values->optInt(i, 0) == 1 && !learned.count(i)) out.insert(i);
  return out;
}

// Skill training on `target`, the card the client picked (z27TrainingTarget).
// The learned skills saved for its serial are this card's.
void applyZiruSkillTrainingResponse(const ObjP& jo, const ObjP& target) {
  try {
    if (!ziruItemType || (*ziruItemType != "SKILL_TRAINING" && *ziruItemType != "SKILL_TRAINING_CATEGORY" && *ziruItemType != "SKILL_TRAINING_SELECT")) return;
    int64_t j = ziruTargetSerial;
    ObjP owned = target;

    zs::Prefs& prefs = sp("almanac_state");
    std::string key = "ziru_skills_" + str(j);
    std::string s = prefs.getString(key, "");
    std::vector<int32_t> al;
    ArrP sk = owned->optJSONArray("additional_skill_id");
    if (sk) {
      for (int i = 0; i < sk->length(); i++) {
        int v = ziruRepairLearnedSkill(sk->optInt(i));

        if (isZiruSkillId(v)) {
          if (vcontains(al, v)) continue;
          if (al.size() >= 5) break;
          al.push_back(v);
        }
      }
    }
    if (!s.empty()) {
      for (auto& part : split(s, ',')) {
        try {
          int v = ziruRepairLearnedSkill(parseInt(part));
          if (isZiruSkillId(v) && !vcontains(al, v) && al.size() < 5) al.push_back(v);
        } catch (const NumberFormat&) {
        }
      }
    }
    std::string pool = "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71";
    if (*ziruItemType == "SKILL_TRAINING_CATEGORY") {
      std::string c = resolveZiruSkillCategory();
      if (c == "UNKNOWN") c = "OTHER";
      if (contains(c, "SHOOT") || contains(c, "FINISH")) pool = "15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30";
      else if (contains(c, "PASS")) pool = "31,32,33,34,35,36,37,38,39,40,41,42,43,46";
      else if (contains(c, "DRIB")) pool = "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14";
      else if (contains(c, "DEF")) pool = "55,56,57,58,59,60,61,62,63,64,65,66";
      else pool = "44,45,47,48,49,50,51,52,53,54,67,68,69,70,71";
    }
    std::vector<std::string> sp_ = split(pool, ',');
    // Only normal skills can be trained: no special skill, no Dummy skill card,
    // nothing the card already has by itself or has already learned.
    std::set<int> cardSkills = z27CardSkillParams(owned);
    auto cardHas = [&](int id) { return isZiruSkillId(id) && cardSkills.count(kZiruSkillParameters[id]) > 0; };
    auto open = [&](int val) { return isZiruTrainableSkill(val) && !cardHas(val) && !vcontains(al, val); };
    std::vector<int> candidates;
    for (const auto& s_item : sp_) {
      int val = parseInt(s_item);
      if (open(val)) candidates.push_back(val);
    }
    // The chosen category has nothing left for this card: any normal skill it
    // neither has nor has learned.
    if (candidates.empty()) {
      for (int val = 0; val < 72; ++val)
        if (open(val)) candidates.push_back(val);
    }
    int slot = ziruTrainingIndex;
    int target = (slot >= 0 && slot < (int)al.size()) ? slot : (al.size() < 5 ? -1 : 0);  // -1: a new slot
    int v = -1;
    if (*ziruItemType == "SKILL_TRAINING_SELECT") {
      int chosen = ziruAbilityId;
      if (chosen == 0 && ziruSkillItemEffect > 0) chosen = ziruSkillItemEffect;
      bool learnedElsewhere = vcontains(al, chosen) && !(target >= 0 && al[target] == chosen);
      if (isZiruTrainableSkill(chosen) && !cardHas(chosen) && !learnedElsewhere) {
        v = chosen;
      } else {
        // Never another skill than the one picked: the training is not successful, the card
        // stays as it is and the item is not used up.
        logRequest("ZIRU_SKILL_V18", "[ZIRU_SKILL_SELECT_REFUSED] serial=" + str(j) + " skill=" + str(chosen) + " cannot be trained");
        jo->put("skill_training_result", newObj()->put("is_success", "NO"));
        z27KeepSkillTrainingItems(jo);
        return;
      }
    } else if (!candidates.empty()) {
      v = candidates[rand() % candidates.size()];
    }
    // Nothing left to learn at all. Retraining a slot keeps the skill it has;
    // a new slot stays empty and the client is told the training did not
    // succeed (it then leaves the card as it is). The item is not used up.
    if (v < 0 && target >= 0) v = al[target];
    if (v < 0) {
      logRequest("ZIRU_SKILL_V18", "[ZIRU_SKILL_NONE_LEFT] serial=" + str(j) + " has every normal skill");
      ObjP res = newObj()->put("is_success", "NO");
      jo->put("skill_training_result", res);
      z27KeepSkillTrainingItems(jo);
      return;
    }

    int replaced = -1;
    if (target >= 0) {
      replaced = al[target];
      al[target] = v;
      logRequest("ZIRU_SKILL_V18", "[ZIRU_SKILL_V18_REPLACE] slot=" + str(target) + " old=" + str(replaced) + " new=" + str(v));
    } else {
      al.push_back(v);
    }

    ArrP ja = newArr();
    for (int skillId : al) ja->put(skillId);
    owned->put("additional_skill_id", ja);

    ArrP params = owned->optJSONArray("parameters");
    if (params && params->length() >= 142) {
      // The replaced skill stops working, unless the card has it by itself.
      if (replaced >= 0 && replaced != v && !cardHas(replaced) && !vcontains(al, replaced)) {
        int pIdx = kZiruSkillParameters[replaced];
        if (isZiruSkillParameter(pIdx) && pIdx < params->length()) params->put(pIdx, 0);
      }
      for (int skillId : al) {
        int pIdx = kZiruSkillParameters[skillId];
        if (isZiruSkillParameter(pIdx) && pIdx < params->length()) params->put(pIdx, 1);
      }
    }
    // A known card ends with exactly its own skills plus the learned ones.
    z27ApplyCardIdentity(owned);
    prefs.edit().putString(key, joinInts(al)).commit();
    saveZiruPlayerToDb(owned);

    logRequest("ZIRU_SKILL_V22", "[ZIRU_SKILL_V22] type=" + *ziruItemType + " serial=" + str(j) + " slot=" + str(target) + " skill awarded: " + str(v));
    ObjP res = jo->optJSONObject("skill_training_result");
    if (!res) { res = newObj(); jo->put("skill_training_result", res); }
    res->put("additional_skill_id", v);
    res->put("is_success", "YES");
    z27KeepSkillTrainingItems(jo);
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_SKILL_V2] response/persistence failed ") + e.what());
    throw;
  }
}

// Skill, booster and position training change exactly the card the client
// picked (z27TrainingTarget). A request naming any other card is answered as a
// training that did not succeed: only that training type's result, with
// is_success NO (Konami's answer template also carries a successful skill
// result, which is removed), nothing is saved and no item is used up.
void z27UseMyclubItem(const ObjP& jo) {
  std::string type = ziruItemType ? *ziruItemType : "";
  bool skill = type == "SKILL_TRAINING" || type == "SKILL_TRAINING_CATEGORY" || type == "SKILL_TRAINING_SELECT";
  bool booster = type == "ABILITY_TRAINING_RANDOM" || type == "ABILITY_TRAINING_SELECT";
  bool position = type == "POSITION_TRAINING" || type == "POSITION_TRAINING_SELECT";
  if (skill || booster || position) {
    ObjP target = z27TrainingTarget(ziruTargetSerial, ziruTargetPlayerId);
    if (!target) {
      for (const char* k : {"skill_training_result", "ability_training_result", "position_training_result"}) jo->remove(k);
      if (skill) {
        jo->put("skill_training_result", newObj()->put("is_success", "NO"));
        z27KeepSkillTrainingItems(jo);
      } else if (booster) {
        jo->put("ability_training_result", newObj()->put("ability_id", 0)->put("ability_Id", 0)->put("is_success", "NO"));
      } else {
        jo->put("position_training_result", newObj()->put("additional_position", "")->put("is_success", "NO"));
      }
      logRequest("Z27_TRAINING_REFUSED", "[Z27_TRAINING_REFUSED] " + type + " id=" + str(ziruTargetPlayerId) + " serial=" + str(ziruTargetSerial) +
                                             ": not the card saved under this serial, nor its own unsaved entry");
    } else if (position) {
      applyZiruPositionTrainingResponse(jo, target);
    } else if (booster) {
      applyZiruBoosterTrainingResponse(jo, target);
    } else {
      applyZiruSkillTrainingResponse(jo, target);
    }
  }
  z27InjectTrainingItems(jo);
}

// Skill training items are never used up.
void z27KeepSkillTrainingItems(const ObjP& jo) {
  ArrP items = jo->optJSONArray("item_list");
  if (!items) return;
  bool hasSel = false;
  for (int i8 = 0; i8 < items->length(); i8++) {
    ObjP it = items->optJSONObject(i8);
    ObjP item;
    if (it && (item = it->optJSONObject("item"))) {
      std::string t = item->optString("item_type", "");
      if (t == "SKILL_TRAINING_SELECT") hasSel = true;
      if (t == "SKILL_TRAINING" || t == "SKILL_TRAINING_CATEGORY" || t == "SKILL_TRAINING_SELECT") it->put("inventory_num", 999999);
    }
  }
  if (!hasSel) {
    ObjP it = newObj();
    it->put("item", newObj()->put("item_type", "SKILL_TRAINING_SELECT")->put("grade", 0)->put("effect", 0));
    it->put("inventory_num", 999999);
    items->put(it);
  }
}

void applyZiruSquadStateToLogin(const ObjP& jo) { z27Restore(jo); }

void auditZiruCriticalAssetsV81() {
  for (const char* n : {"responses/CmdLogin.json", "responses/ZIRU_PLAYER_RECORDS.json", "responses/ZIRU_PACKS_CONFIG.json",
                        "responses/CMD_USE_MYCLUB_ITEM.json", "responses/ZIRU_ML_COMPETITION_REGISTRY.json",
                        "responses/ZIRU_ML_SCHEDULE_113.json", "responses/ZIRU_GET_GAMERESULT_LIST.json",
                        "responses/ZIRU_DIVISION_OPPONENT_POOL.json", "responses/CmdGetMyclubEntryInfo.json"}) {
    if (!assets.exists(n)) zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V81_ASSET_MISSING] ") + n);
  }
  zs::logi("OFFLINE_SERVER", "[ZIRU_V81_BOOT_AUDIT] critical asset scan complete");
}

void auditZiruWorldCupAssetsV86() {
  ObjP a = loadZiruJsonAssetObject("responses/ZIRU_WC_CONFIG.json");
  ObjP b = loadZiruJsonAssetObject("responses/ZIRU_WC_SPECIAL_POOL.json");
  zs::logi("ZIRU_WC", "[ZIRU_WC_V86_FOUNDATION] config=" + str(a ? a->length() : 0) + " pool=" + str(b ? b->length() : 0));
}
