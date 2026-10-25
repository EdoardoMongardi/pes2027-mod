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

    // The client puts gameplayer_sell_gp on the card after any booster answer (QuitCommand
    // 0x104774db4); a missing one reads as 0. The card's sell value does not change.
    if (r8 && r8->has("sell_gp")) r13->put("gameplayer_sell_gp", r8->optLong("sell_gp", 0));
    int32_t r2;
    if (r0 == "ABILITY_TRAINING_RANDOM") {
      r2 = z28RandomBooster(isGk);
    } else if (r0 == "ABILITY_TRAINING_SELECT") {
      r2 = ziruAbilityId;
      if (r2 <= 0) {
        r13->put("ability_training_result", newObj()->put("ability_id", 0)->put("ability_Id", 0)->put("is_success", "NO"));
        logRequest("ZIRU_BOOSTER_V23", "[ZIRU_BOOSTER_V23_FAIL] missing target/ability");
        return;
      }
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

// ---- Boxes: their cards (ZIRU_PACKS_CONFIG.json) and what is left in each ----
// The box config, read at most once per request (serve() clears it).
ObjP ziruPacksCache;
int ziruPacksParses = 0;  // how often ZIRU_PACKS_CONFIG.json was parsed (checked by the tests)

ObjP ziruPacksConfig() {
  if (!ziruPacksCache) {
    ziruPacksParses++;
    ziruPacksCache = Object::parse(readAsset("responses/ZIRU_PACKS_CONFIG.json"));
  }
  return ziruPacksCache;
}

// The box with this agent id in the config, or null when the agent is not one of the boxes.
ObjP ziruPack(int64_t agent) {
  ArrP packs = ziruPacksConfig()->optJSONArray("packs");
  for (int i = 0; packs && i < packs->length(); i++) {
    ObjP p = packs->optJSONObject(i);
    if (p && p->optLong("agent_id") == agent) return p;
  }
  return nullptr;
}

// A box's cards, the ones a draw can give: its player_ids that have a player record, each card once. A card
// without a record cannot be drawn (the draw would hand out a copy of the template card with its id), so it is
// left out, and logged once.
std::set<std::pair<int64_t, int64_t>> ziruBoxCardLogged;  // (agent id, card id) already reported

ArrP ziruBoxCards(const ObjP& pack) {
  ArrP ids = pack ? pack->optJSONArray("player_ids") : nullptr;
  ObjP records = ziruPlayerRecords();
  ArrP cards = newArr();
  std::set<int64_t> seen;
  for (int i = 0; ids && i < ids->length(); i++) {
    int64_t id = ids->optLong(i);
    if (!seen.insert(id).second) continue;
    if (records && records->has(str(id))) {
      cards->put(ids->opt(i));
    } else if (ziruBoxCardLogged.insert({pack->optLong("agent_id"), id}).second) {
      logRequest("ZIRU_BOX_CARD", "[ZIRU_BOX_CARD] agent_id=" + str(pack->optLong("agent_id")) + " card " + str(id) +
                                      " has no player record: left out of the box");
    }
  }
  return cards;
}

// A short fingerprint of a box's cards, kept with what is left of the box: the number of cards and the
// 64-bit FNV-1a hash of the sorted card ids, so a change in the box's cards is noticed and a new order is not.
std::string ziruBoxSignature(const ArrP& cards) {
  static constexpr uint64_t kFnvOffsetBasis = 14695981039346656037ULL, kFnvPrime = 1099511628211ULL;
  std::vector<int64_t> ids;
  for (int i = 0; i < cards->length(); i++) ids.push_back(cards->optLong(i));
  std::sort(ids.begin(), ids.end());
  uint64_t h = kFnvOffsetBasis;
  for (int64_t id : ids)
    for (char c : std::to_string(id) + ",") { h ^= (unsigned char)c; h *= kFnvPrime; }
  return std::to_string(ids.size()) + ":" + std::to_string(h);
}

// The card ids the user has: every saved card, every drawn card and the starter cards (also under the id the iOS
// client knows a starter card by), leaving out sold ones. Built at most once per request (serve() clears it, and a
// draw clears it again).
std::optional<std::set<int64_t>> ziruOwnedCache;

const std::set<int64_t>& ziruOwnedCardIds() {
  if (ziruOwnedCache) return *ziruOwnedCache;
  std::set<int64_t> own;
  auto add = [&](const ObjP& card) {
    ObjP gp = card && card->optJSONObject("gameplayer") ? card->getJSONObject("gameplayer") : card;
    ObjP gid = gp ? gp->optJSONObject("gameplayer_id") : nullptr;
    if (gid && !isZiruPlayerSold(gid->optLong("serial", 0))) own.insert(gid->optLong("id", 0));
  };
  for (auto& row : ziruDb->loadAllPlayerRows()) {
    try { add(Object::parse(row.second)); } catch (const std::exception&) {}
  }
  std::string recruited = sp("almanac_state").getString("recruited_players", "");
  if (!recruited.empty()) {
    ArrP list = Array::parse(recruited);
    for (int i = 0; i < list->length(); i++) add(list->optJSONObject(i));
  }
  for (const auto& kv : z27Starters().bySerialId)
    if (!isZiruPlayerSold(kv.first.first)) own.insert(kv.first.second);
  ziruOwnedCache = std::move(own);
  return *ziruOwnedCache;
}

std::string ziruRemainingKey(int64_t agent) { return "ziru_remaining_" + str(agent); }
std::string ziruBoxSignatureKey(int64_t agent) { return "ziru_box_signature_" + str(agent); }

// What is left in a box: the cards a draw can give. Every reader of a box's saved list uses this.
// A box this save has not seen yet, and a box whose cards changed since its list was saved (an older build's boxes
// under a reused id, or new box contents), start again as the box's cards minus the cards the user already has, so
// nothing the user has is offered again and no card of another box comes out of it. A box the user reset holds all
// its cards again.
ArrP ziruBoxRemaining(int64_t agent, const ArrP& cards) {
  zs::Prefs& prefs = sp("almanac_state");
  std::string key = ziruRemainingKey(agent), signature = ziruBoxSignature(cards);
  std::string saved = prefs.getString(key, "");
  if (prefs.getString(ziruBoxSignatureKey(agent), "") == signature)
    return Array::parse(saved.empty() ? cards->toString() : saved);
  const std::set<int64_t>& own = ziruOwnedCardIds();
  ArrP left = newArr();
  for (int i = 0; i < cards->length(); i++)
    if (!own.count(cards->optLong(i))) left->put(cards->opt(i));
  prefs.edit().putString(key, left->toString()).putString(ziruBoxSignatureKey(agent), signature).commit();
  logRequest("ZIRU_BOX_MIGRATE", "[ZIRU_BOX_MIGRATE] agent_id=" + str(agent) + " cards=" + str(cards->length()) + " left=" +
                                     str(left->length()) + (saved.empty() ? " (new box)" : " (changed box)"));
  return left;
}

// A box reply carries five counts (remaining_player_count, base_player_count); the client shows their sum
// (AUCMenuMyClubAgentView::GetRemainingPlayerNumStr 0x100d8c1f4). The mod keeps a box's whole count in the
// third one, where the shipped box lists have it.
static constexpr int kZiruBoxCountSlots = 5, kZiruBoxCountSlot = 2;

ArrP ziruBoxCounts(int count) {
  ArrP c = newArr();
  for (int i = 0; i < kZiruBoxCountSlots; i++) c->put(i == kZiruBoxCountSlot ? count : 0);
  return c;
}

int countZiruBox(const ArrP& counts) {
  int n = 0;
  for (int i = 0; counts && i < counts->length(); i++) n += counts->optInt(i);
  return n;
}

void applyZiruDynamicRecruit(const ObjP& jo) {
  try {
    int64_t j = ziruLastAgentId;
    if (j <= 0) return;
    int32_t i = ziruLastCount;
    if (i <= 0) i = 1;
    ObjP pack = ziruPack(j);
    if (!pack) { zs::logi("OFFLINE_SERVER", "[ZIRU_V10_DRAW] agent_id not in ZIRU config; leaving template unchanged"); return; }
    ArrP ids = ziruBoxCards(pack);
    zs::Prefs& prefs = sp("almanac_state");
    std::string key = ziruRemainingKey(j);
    ArrP remaining = ziruBoxRemaining(j, ids);
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
    ziruOwnedCache.reset();  // the user has the drawn cards now
    std::string msg = "[ZIRU_V10_DRAW] count=" + str(i) + " remaining=" + str(remaining->length());
    zs::logi("OFFLINE_SERVER", msg);
    logRequest("ZIRU_V10_DRAW", msg);
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_V10_DRAW_ERR] dynamic recruit failed ") + e.what());
    throw;
  }
}

// Name of each position level in parameters[1..13], as additional_position and
// position_training_result carry it (CommandCamPosition literals).
static constexpr const char* kZiruPositionNames[14] = {"", "CF", "ST", "LWG", "RWG", "OMF", "DMF", "CMF", "LMF", "RMF", "CB", "LSB", "RSB", "GK"};

static inline int z27GetPositionIndexByName(const std::string& name) {
  for (int i = 1; i <= 13; i++) {
    if (name == kZiruPositionNames[i]) return i;
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

// ---- Learned positions, as Konami's server and the 11.1.1 client keep them ----
// A card plays each position at level 0, 1 or 2 (parameters[1..13]) and has two slots for
// learned positions (additional_position). A learned position is the card's own level there
// plus one for each slot holding it, at most 2:
// - after a successful training the client itself takes one level off the position the slot
//   held and adds one to the new position (CommandObjectCmdUseMyclubItem::QuitCommand,
//   0x104774eac..0x104774f24);
// - it counts a card's own level as its level minus the slots holding that position
//   (UtilityMyClubTraining::GetPositionTrainableNum, 0x1048a4df0), and the select screen
//   previews a pick as one level up (MyClubMyTeamPlayerTrainingPositionSelect::GetPositionArea,
//   0x104683354);
// - Konami's own login data has three cards with a learned position: OMF at the card's own
//   level 1 is at 2, LWG and RMF at the card's own level 0 are at 1.

// How many of the slots in `learned` hold the position at parameters index k.
static int z27SlotsHolding(const ArrP& learned, int k) {
  int n = 0;
  for (int i = 0; learned && i < learned->length() && i < 2; i++)
    if (z27GetPositionIndexByName(learned->optString(i, "")) == k) n++;
  return n;
}

// The card's own position levels, parameters[1..13] (index 0 unused): the levels of what the
// card was made from, without the positions learned there. That is the starter entry listed
// under its serial (also when the client knows the card by another id), else its record, else
// its starter or rental entry (z27CardSource), else the card itself.
void z27OwnPositionLevels(const ObjP& p, int own[14]) {
  ObjP gid = p ? p->optJSONObject("gameplayer_id") : nullptr;
  int64_t id = gid ? gid->optLong("id", 0) : 0, serial = gid ? gid->optLong("serial", 0) : 0;
  const ZiruStarters& starters = z27Starters();
  ObjP source;
  auto listed = starters.bySerialId.find({serial, id});
  if (serial > 0 && listed != starters.bySerialId.end()) source = listed->second;
  if (!source) source = z27CardSource(id);
  auto upgraded = starters.byUpgradedId.find(id);
  if (!source && upgraded != starters.byUpgradedId.end()) source = upgraded->second;
  ArrP values = source ? source->optJSONArray("parameters") : nullptr;
  if (!values || values->length() < 14) {
    source = p;
    values = p ? p->optJSONArray("parameters") : nullptr;
  }
  ArrP learned = source ? source->optJSONArray("additional_position") : nullptr;
  own[0] = 0;
  for (int k = 1; k <= 13; k++) own[k] = values && values->length() >= 14 ? std::max(0, values->optInt(k, 0) - z27SlotsHolding(learned, k)) : 0;
}

// Sets the card's position levels from its own levels and the slots it has.
void z27PutPositionLevels(const ObjP& p, const int own[14]) {
  ArrP learned = p->optJSONArray("additional_position");
  for (const char* key : {"parameters", "original_parameters"}) {
    ArrP values = p->optJSONArray(key);
    if (!values || values->length() < 14) continue;
    for (int k = 1; k <= 13; k++) values->put(k, std::min(2, own[k] + z27SlotsHolding(learned, k)));
  }
}

// A card that learned positions shows them as the client and Konami's server have them. Builds
// before fix3 saved a learned position at level 2 whatever the card's own level, and a replaced
// one at the record's level or 0.
void z27ApplyLearnedPositions(const ObjP& p) {
  ArrP learned = p ? p->optJSONArray("additional_position") : nullptr;
  ArrP values = p ? p->optJSONArray("parameters") : nullptr;
  if (!learned || learned->length() == 0 || !values || values->length() < 14) return;
  int own[14];
  z27OwnPositionLevels(p, own);
  z27PutPositionLevels(p, own);
}

// Whether the client offers the position at parameters index k for `slot`, as
// MyClubMyTeamPlayerTrainingPositionSelect::AvailablePositionTraining (0x10468321c) decides with
// UtilityMyClubTraining::GetTrainablePositionList (0x1048a4ba4) and AddToPositionList
// (0x1048a4cfc): below level 2, not the position this slot holds, and either played (level 1)
// or next to the card's registered position (parameters[0], a Position number). The positions
// next to each one are the table the client builds at 0x1048a5d44.
static bool z27PositionOffered(const ArrP& values, const ArrP& learned, int slot, int k) {
  static const std::vector<int> nextTo[13] = {{}, {2, 3, 4}, {1, 6}, {1, 7}, {1, 5, 6, 7}, {4, 6, 7, 8},
      {2, 4, 5, 8, 9}, {3, 4, 5, 8, 10}, {6, 7, 5, 11}, {6, 11, 12}, {7, 11, 12}, {9, 10, 8, 12}, {9, 10, 11}};
  if (k < 1 || k > 13) return false;
  int level = values->optInt(k, 0);
  if (level > 1) return false;
  if (learned && slot < learned->length() && z27GetPositionIndexByName(learned->optString(slot, "")) == k) return false;
  int registered = values->optInt(0, -1);
  if (level != 0 && k != z27PositionParameter(registered)) return true;
  if (registered < 0 || registered > 12) return false;
  for (int position : nextTo[registered])
    if (z27PositionParameter(position) == k) return true;
  return false;
}

// Position training on `target`, the card the client picked (z27TrainingTarget). Position
// Training (Select) teaches the position picked, Position Training a random one, in either case
// only a position the client offers for that slot; otherwise the answer is not successful,
// nothing changes and the item is not used up.
void applyZiruPositionTrainingResponse(const ObjP& jo, const ObjP& target) {
  try {
    if (!(ziruItemType && (*ziruItemType == "POSITION_TRAINING" || *ziruItemType == "POSITION_TRAINING_SELECT"))) return;
    bool select = *ziruItemType == "POSITION_TRAINING_SELECT";
    int64_t j = ziruTargetSerial;
    ObjP p = target;
    ArrP params = p ? p->optJSONArray("parameters") : nullptr;
    auto refuse = [&](const std::string& why) {
      jo->put("position_training_result", newObj()->put("additional_position", "")->put("is_success", "NO"));
      z27KeepPositionTrainingItems(jo);
      logRequest("ZIRU_POSITION_V26", "[ZIRU_POSITION_REFUSED] type=" + *ziruItemType + " serial=" + str(j) + " " + why);
    };
    if (j <= 0 || !params || params->length() < 14) {
      refuse("missing/invalid player state");
      return;
    }
    ArrP add = p->optJSONArray("additional_position");
    if (!add) add = newArr();
    // The client opens slot 1 only once slot 0 holds a position (MyClubMyTeamPlayerTrainingPosition::SetupPlayerData, 0x104682850).
    int slot = ziruTrainingIndex;
    if (slot != 0 && slot != 1) slot = (add->length() == 0) ? 0 : 1;
    if (add->length() == 0) slot = 0;

    // The levels as the client shows them.
    int own[14];
    z27OwnPositionLevels(p, own);
    ArrP shown = newArr();
    shown->put(params->optInt(0, 0));
    for (int k = 1; k <= 13; k++) shown->put(std::min(2, own[k] + z27SlotsHolding(add, k)));

    int chosen = -1;
    if (select) {
      if (!z27PositionOffered(shown, add, slot, ziruPickedPosition)) {
        refuse("slot=" + str(slot) + " position=" + str(ziruPickedPosition) + " cannot be learned");
        return;
      }
      chosen = ziruPickedPosition;
    } else {
      std::vector<int> candidates;
      for (int k = 1; k <= 13; k++)
        if (z27PositionOffered(shown, add, slot, k)) candidates.push_back(k);
      if (candidates.empty()) {
        refuse("slot=" + str(slot) + " no position left to learn");
        return;
      }
      chosen = candidates[rand() % candidates.size()];
    }
    std::string chosen_pos = kZiruPositionNames[chosen];
    if (slot < add->length()) add->put(slot, chosen_pos);
    else add->put(chosen_pos);
    p->put("additional_position", add);
    z27PutPositionLevels(p, own);
    saveZiruPlayerToDb(p);

    jo->put("position_training_result", newObj()->put("additional_position", chosen_pos)->put("is_success", "YES"));
    z27KeepPositionTrainingItems(jo);
    logRequest("ZIRU_POSITION_V26", "[ZIRU_POSITION_V26] type=" + *ziruItemType + " serial=" + str(j) + " slot=" + str(slot) +
                                        " position=" + chosen_pos + " level=" + str(params->optInt(chosen, 0)));
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_POSITION_V26_ERR] ") + e.what());
    logRequest("ZIRU_POSITION_V26", std::string("[ZIRU_POSITION_V26_ERR] ") + e.what());
  }
}

void applyZiruProcurableList(const ObjP& jo) {
  try {
    int64_t i = ziruLastAgentId;
    if (i <= 0) return;
    ObjP pack = ziruPack(i);
    if (!pack) return;
    ArrP ids = ziruBoxRemaining(i, ziruBoxCards(pack));
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

// ---- The box list the client gets ----
// Every list of boxes the client gets (agent_list and agent_price_list in the box list reply and in the main menu,
// agent_list in the draw reply) is made here from the box config, so these lists cannot differ from each other or
// from the boxes a draw uses. Each box entry and price entry starts from a template: the shipped box list's
// (CmdGetMyclubAgentlist.json) entry with the same agent id, else its first entry. The config gives the box's id,
// title, cards and counts, and when it has them, its label (subcategory), colours, banner pictures and featured
// cards.

// The shipped box list, read at most once per request (serve() clears it).
ObjP ziruBoxListCache;

ObjP ziruBoxListTemplate() {
  if (!ziruBoxListCache) ziruBoxListCache = nn(loadZiruJsonAssetObject("responses/CmdGetMyclubAgentlist.json"));
  return ziruBoxListCache;
}

// The template of a box's entry in one of the shipped box list's lists: the entry with the box's agent id, else
// the first entry (null when the list is empty). `own` tells whether it is the box's own entry.
ObjP ziruBoxTemplate(const std::string& list, int64_t agent, bool& own) {
  ArrP entries = ziruBoxListTemplate()->optJSONArray(list);
  ObjP first;
  own = false;
  for (int i = 0; entries && i < entries->length(); i++) {
    ObjP e = entries->optJSONObject(i);
    if (!e) continue;
    if (!first) first = e;
    if (e->optLong("agent_id") == agent) {
      own = true;
      return e;
    }
  }
  return first;
}

// The featured cards of a box (pickup_list, banner_a/c_pickup_list, pickup_cardtype_list): the config's
// pickup_ids, else the ones of the box's own template entry, keeping only cards of this box. When none is left,
// the box's first cards, as many as the template shows.
std::set<std::pair<int64_t, int64_t>> ziruBoxPickupLogged;  // (agent id, card id) already reported

ArrP ziruBoxPickups(const ObjP& pack, const ArrP& cards, const ObjP& base, bool ownTemplate) {
  int64_t agent = pack->optLong("agent_id");
  std::set<int64_t> inBox;
  for (int i = 0; i < cards->length(); i++) inBox.insert(cards->optLong(i));
  ArrP wanted = pack->optJSONArray("pickup_ids");
  if (!wanted && ownTemplate && base) {
    wanted = newArr();
    ArrP shown = base->optJSONArray("pickup_cardtype_list");
    for (int i = 0; shown && i < shown->length(); i++)
      if (ObjP c = shown->optJSONObject(i)) wanted->put(c->optLong("id"));
  }
  ArrP picks = newArr();
  std::set<int64_t> seen;
  for (int i = 0; wanted && i < wanted->length(); i++) {
    int64_t id = wanted->optLong(i);
    if (inBox.count(id)) {
      if (seen.insert(id).second) picks->put((long long)id);
    } else if (ziruBoxPickupLogged.insert({agent, id}).second) {
      logRequest("ZIRU_BOX_PICKUP", "[ZIRU_BOX_PICKUP] agent_id=" + str(agent) + " featured card " + str(id) +
                                        " is not a card of the box: not shown");
    }
  }
  ArrP shown = base ? base->optJSONArray("pickup_list") : nullptr;
  int count = shown ? shown->length() : 0;
  for (int i = 0; picks->length() == 0 && i < cards->length() && i < count; i++) picks->put(cards->opt(i));
  return picks;
}

// A box's entry in the box list, or null when the box has no card to draw (it could never be opened).
ObjP ziruBoxEntry(const ObjP& pack) {
  int64_t agent = pack->optLong("agent_id");
  ArrP cards = ziruBoxCards(pack);
  if (cards->length() == 0) return nullptr;
  bool own = false;
  ObjP base = ziruBoxTemplate("agent_list", agent, own);
  // A copy of the template, without its featured cards (they are made below).
  static const std::set<std::string> kFeatured = {"pickup_list", "banner_a_pickup_list", "banner_c_pickup_list", "pickup_cardtype_list"};
  ObjP a = newObj();
  for (const auto& kv : base ? base->kv : std::vector<std::pair<std::string, Value>>{}) {
    if (kFeatured.count(kv.first)) continue;
    Value v = kv.second;
    if (v.t == Value::OBJ) v = Value(Object::parse(v.o->toString()));
    if (v.t == Value::ARR) v = Value(Array::parse(v.a->toString()));
    a->put(kv.first, v);
  }
  a->put("agent_id", (long long)agent);
  for (const char* key : {"title", "subcategory", "banner_file_c_name", "banner_file_d_name", "text_color_a", "text_color_b"}) {
    Value v = pack->opt(key);
    if (v.t == Value::OBJ) v = Value(Object::parse(v.o->toString()));
    if (!v.isJavaNull()) a->put(key, v);
  }
  int total = cards->length(), left = ziruBoxRemaining(agent, cards)->length();
  a->put("player_list_total", total);
  a->put("probability", str(total));  // the shipped box lists give the box's number of cards here
  a->put("base_player_count", ziruBoxCounts(total));
  a->put("remaining_player_count", ziruBoxCounts(left));
  ArrP pickupIds = ziruBoxPickups(pack, cards, base, own);
  ArrP pickups = newArr(), cardTypes = newArr();
  for (int i = 0; i < pickupIds->length(); i++) {
    ObjP w = getZiruPlayerWrapper(pickupIds->opt(i), false);
    ObjP gp = w ? w->optJSONObject("gameplayer") : nullptr;
    if (!gp) continue;
    pickups->put(gp);
    cardTypes->put(newObj()->put("id", (long long)pickupIds->optLong(i))->put("serial", 0));
  }
  a->put("pickup_list", pickups);
  a->put("banner_a_pickup_list", Array::parse(pickups->toString()));
  a->put("banner_c_pickup_list", Array::parse(pickups->toString()));
  a->put("pickup_cardtype_list", cardTypes);
  return a;
}

// A box's price entry: the template's, under the box's agent id.
ObjP ziruBoxPrice(int64_t agent) {
  bool own = false;
  ObjP base = ziruBoxTemplate("agent_price_list", agent, own);
  ObjP p = base ? Object::parse(base->toString()) : newObj();
  p->put("agent_id", (long long)agent);
  return p;
}

// Puts the box list into a reply: agent_list, and agent_price_list when `prices`. Every box of the config is
// listed once, by its order (a box without one by its place in the file), except a box with no card to draw.
// When the config cannot be read, the reply keeps the lists it has.
void ziruPutBoxList(const ObjP& jo, bool prices) {
  try {
    ArrP packs = ziruPacksConfig()->optJSONArray("packs");
    std::vector<std::pair<int64_t, ObjP>> ordered;
    for (int i = 0; packs && i < packs->length(); i++)
      if (ObjP p = packs->optJSONObject(i)) ordered.push_back({p->optLong("order", i + 1), p});
    std::stable_sort(ordered.begin(), ordered.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    ArrP agents = newArr(), priceList = newArr();
    std::set<int64_t> listed;
    for (const auto& entry : ordered) {
      int64_t agent = entry.second->optLong("agent_id");
      if (!listed.insert(agent).second) continue;  // the first box with an id is the one a draw uses (ziruPack)
      ObjP a = ziruBoxEntry(entry.second);
      if (!a) continue;
      agents->put(a);
      if (prices) priceList->put(ziruBoxPrice(agent));
      logRequest("ZIRU_V12_2_REMAINING", "[ZIRU_V12_2_REMAINING] agent_id=" + str(agent) + " remaining=" +
                                             str(countZiruBox(a->getJSONArray("remaining_player_count"))));
    }
    jo->put("agent_list", agents);
    if (prices) jo->put("agent_price_list", priceList);
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[ZIRU_BOX_LIST_ERR] ") + e.what());
    logRequest("ZIRU_BOX_LIST", std::string("[ZIRU_BOX_LIST_ERR] the box list is left as it is: ") + e.what());
  }
}

void applyZiruResetBox(const ObjP& jo) {
  try {
    int64_t j = ziruLastAgentId;
    if (j > 0) {
      // A reset box holds all its cards again, the ones the user has included (ziruBoxRemaining).
      auto edit = sp("almanac_state").edit();
      edit.remove(ziruRemainingKey(j));
      if (ObjP pack = ziruPack(j)) edit.putString(ziruBoxSignatureKey(j), ziruBoxSignature(ziruBoxCards(pack)));
      edit.commit();
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
      ArrP params = p->optJSONArray("parameters");
      // Konami's game offers the goalkeeping skills only to a goalkeeper (its registered position,
      // parameters[0], is GK), but random Skill Training in builds up to fix2 also taught them to
      // outfield cards. Such a skill is removed: the later ones move up one slot, as when the player
      // deletes a skill, and its flag is cleared unless the card has the skill by itself.
      bool goalkeepingRemoved = false;
      if (params && params->length() > 0 && !isZiruGoalkeeperCard(params) &&
          std::any_of(al.begin(), al.end(), [](int v) { return isZiruGoalkeeperSkill(v); })) {
        std::set<int> cardSkills = z27CardSkillParams(p);
        std::vector<int32_t> kept, removed;
        for (int v : al) (isZiruGoalkeeperSkill(v) ? removed : kept).push_back(v);
        for (int v : removed) {
          int index = kZiruSkillParameters[v];
          if (isZiruSkillParameter(index) && index < params->length() && !cardSkills.count(index)) params->put(index, 0);
        }
        logRequest("Z27_GK_SKILL_REMOVED", "[Z27_GK_SKILL_REMOVED] serial=" + str(serial) + " id=" + str(pid) + " outfield card: removed " +
                                               joinInts(removed) + ", learned " + joinInts(al) + " -> " + joinInts(kept));
        al = kept;
        goalkeepingRemoved = true;
      }
      ArrP ja = newArr();
      for (int v : al) ja->put(v);
      p->put("additional_skill_id", ja);

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
      if ((!al.empty() || goalkeepingRemoved) && prefs.getString("ziru_skills_" + str(serial), "") != joinInts(al)) {
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

// The learned skills of `card`, saved under `serial`, in slot order: its additional_skill_id, then
// any skill saved for the serial that is not listed there, at most 5.
std::vector<int32_t> z27LearnedSkills(const ObjP& card, int64_t serial) {
  std::vector<int32_t> al;
  ArrP sk = card->optJSONArray("additional_skill_id");
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
  std::string s = sp("almanac_state").getString("ziru_skills_" + str(serial), "");
  if (!s.empty()) {
    for (auto& part : split(s, ',')) {
      try {
        int v = ziruRepairLearnedSkill(parseInt(part));
        if (isZiruSkillId(v) && !vcontains(al, v) && al.size() < 5) al.push_back(v);
      } catch (const NumberFormat&) {
      }
    }
  }
  return al;
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
    std::vector<int32_t> al = z27LearnedSkills(owned, j);
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
    // Only the skills the card can learn (the list its player data shows): no special
    // skill, no Dummy skill card, no goalkeeping skill for an outfield card, nothing the
    // card already has by itself or has already learned.
    std::set<int> cardSkills = z27CardSkillParams(owned);
    bool goalkeeper = isZiruGoalkeeperCard(owned->optJSONArray("parameters"));
    auto cardHas = [&](int id) { return isZiruSkillId(id) && cardSkills.count(kZiruSkillParameters[id]) > 0; };
    auto open = [&](int val) { return isZiruLearnableSkill(val, goalkeeper) && !cardHas(val) && !vcontains(al, val); };
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
      // A skill in any learned slot, the slot picked included, is one the client never offers
      // (UtilityMyClubTraining::HasSkill 0x1048a5aec looks at all 5 slots).
      bool learned = vcontains(al, chosen);
      if (isZiruLearnableSkill(chosen, goalkeeper) && !cardHas(chosen) && !learned) {
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

// Skill Training: the player deletes a learned skill. The 11.1.1 client sends the card and the slot
// (CmdDeleteMyclubSkillOption::PackJson 0x103b35d30: gameplayer_id, delete_skill_index). On NOERR it
// turns off the flag of the skill in that slot and moves the later slots up one
// (CommandObjectCmdDeleteMyclubSkill::QuitCommand 0x104765ae4 ->
// MyClubGamePlayer::DeleteAdditionalSkill 0x1047e8340). The server does the same to the card it keeps,
// in both places learned skills are saved, so the next training and the next login show what the
// client shows. One difference is on purpose: when the card also has the skill by itself, the game
// turns its flag off all the same, but the server keeps it on, so the card keeps its own skill, and
// the game shows it again after the next login.
// A request for a card that is not the one saved under that serial (z27TrainingTarget), or for a slot
// without a learned skill, changes nothing and gets an error answer shaped like z27Error's. On any
// result other than NOERR the client ends the command as failed without running QuitCommand
// (CommandObjectBase::actionMain 0x1048d3928), so its card stays as it is too. "ERROR" is not one of
// the results the client treats as fatal (CommandObjectBase::IsFatalError 0x1048d296c), so it is an
// ordinary error and the player stays in the menu; only fatal ones go back to the top menu
// (onlinemode::CommandErrorManager::JumpForTopMenuFatal 0x10486cce0).
Response handleZiruDeleteMyclubSkill() {
  ObjP req = decodeZiruRequestObject(ziruLastRequestBody);
  long long rq = req ? req->optLong("rqid", 1) : 1;
  ObjP gid = req ? req->optJSONObject("gameplayer_id") : nullptr;
  int64_t id = gid ? gid->optLong("id", 0) : 0, serial = gid ? gid->optLong("serial", 0) : 0;
  int slot = req && req->has("delete_skill_index") ? req->optInt("delete_skill_index", -1) : -1;
  auto refuse = [&]() {
    return serveObjectAsEncryptedMsgPack(
        newObj()->put("msgid", "CMD_DELETE_MYCLUB_SKILL")->put("rqid", rq)->put("result", "ERROR")->put("errcode", "ERROR"));
  };
  std::string what = "SKILL_DELETE id=" + str(id) + " serial=" + str(serial) + " slot=" + str(slot);
  ObjP card = z27TrainingTarget(serial, id);
  if (!card) {
    logRequest("Z27_TRAINING_REFUSED", "[Z27_TRAINING_REFUSED] " + what + ": not the card saved under this serial, nor its own unsaved entry");
    return refuse();
  }
  std::vector<int32_t> al = z27LearnedSkills(card, serial);
  if (slot < 0 || slot >= (int)al.size()) {
    logRequest("Z27_TRAINING_REFUSED", "[Z27_TRAINING_REFUSED] " + what + ": the card has " + str((int32_t)al.size()) + " learned skills");
    return refuse();
  }
  std::set<int> cardSkills = z27CardSkillParams(card);
  int32_t removed = al[slot];
  al.erase(al.begin() + slot);
  ArrP ja = newArr();
  for (int skill : al) ja->put(skill);
  card->put("additional_skill_id", ja);
  ArrP params = card->optJSONArray("parameters");
  int index = kZiruSkillParameters[removed];
  // The card's own skill stays on (see above).
  if (params && isZiruSkillParameter(index) && index < params->length() && !cardSkills.count(index)) params->put(index, 0);
  // A known card ends with exactly its own skills plus the learned ones.
  z27ApplyCardIdentity(card);
  sp("almanac_state").edit().putString("ziru_skills_" + str(serial), joinInts(al)).commit();
  saveZiruPlayerToDb(card);
  logRequest("ZIRU_SKILL_DELETE", "[ZIRU_SKILL_DELETE] serial=" + str(serial) + " slot=" + str(slot) + " skill=" + str(removed) +
                                      " learned now: " + joinInts(al));
  return serveObjectAsEncryptedMsgPack(newObj()->put("result", "NOERR")->put("rqid", rq)->put("msgid", "CMD_DELETE_MYCLUB_SKILL"));
}

// Skill, booster and position training change exactly the card the client
// picked (z27TrainingTarget). A request naming any other card is answered as a
// training that did not succeed: only that training type's result, with
// is_success NO, nothing is saved and no item is used up.
// Every answer carries only the result of the item used: the client reads only
// that one (CommandObjectCmdUseMyclubItem::QuitCommand 0x104774bc8 picks skill,
// booster or position by the item type). Konami's answer template is a skill
// training answer with a successful skill result of its own, which is removed.
void z27UseMyclubItem(const ObjP& jo) {
  std::string type = ziruItemType ? *ziruItemType : "";
  bool skill = type == "SKILL_TRAINING" || type == "SKILL_TRAINING_CATEGORY" || type == "SKILL_TRAINING_SELECT";
  bool booster = type == "ABILITY_TRAINING_RANDOM" || type == "ABILITY_TRAINING_SELECT";
  bool position = type == "POSITION_TRAINING" || type == "POSITION_TRAINING_SELECT";
  for (const char* k : {"skill_training_result", "ability_training_result", "position_training_result"}) jo->remove(k);
  if (skill || booster || position) {
    ObjP target = z27TrainingTarget(ziruTargetSerial, ziruTargetPlayerId);
    if (!target) {
      if (skill) {
        jo->put("skill_training_result", newObj()->put("is_success", "NO"));
        z27KeepSkillTrainingItems(jo);
      } else if (booster) {
        jo->put("ability_training_result", newObj()->put("ability_id", 0)->put("ability_Id", 0)->put("is_success", "NO"));
        // The client puts gameplayer_sell_gp on its card after a NO too (QuitCommand 0x104774db4): the sell
        // value of the card it named, looked up by that exact card (z27Player), not of the card saved here.
        ObjP named = z27Player(newObj()->put("id", (long long)ziruTargetPlayerId)->put("serial", (long long)ziruTargetSerial));
        if (named && named->has("sell_gp")) jo->put("gameplayer_sell_gp", named->optLong("sell_gp", 0));
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
