// ---- Saves written while the starter cards shared 11 serials ----
// Build 138 (75288ee) shipped the 70 starter cards under 11 serials. Everything
// the server keeps for a card is keyed by its serial (player row, careerplan
// points, learned skills, sold flag, squad references), so a save written then
// holds one card per old serial: the first card listed there (its "row card"),
// carrying every training done on any card of that serial. Konami's own data
// gives every card a serial of its own (kZiruStarterSerials, starter_serials.inc).
// This moves such a save once, before any request is handled:
//   * each row card goes to its own new serial with its learned skills, boosters,
//     positions, level and careerplan points;
//   * the other starter cards start with their starter values (their rows are
//     made from CmdLogin.json at the next login);
//   * saved squads and the MyLeague special squad point at the new serials; a
//     card listed twice under one old serial: the first reference in a squad
//     gets the copy Konami's own squad uses, the second one the other copy;
//   * a sold old serial keeps every card that was listed under it hidden, as
//     build 138 showed it;
//   * pack cards (serials 1..N) and recruited_players are not touched.
// Everything removed or rewritten is kept in meta z27_starter_serials_backup.
// The database part is one transaction. The learned-skill prefs are written
// right after it from a plan kept in meta (z27_starter_serials_prefs), which is
// applied again at the next start if the app stops in between.
// It runs only with the new starter data (every card of the table listed under
// its new serial); with the old data it waits and changes nothing.

bool ziruSaveChecked = false;            // z27MigrateSave finished in this process
bool ziruSerialWaitingLogged = false;

// Once per process, before any handler reads the save.
void z27MigrateSave() {
  if (ziruSaveChecked) return;
  try {
    z28MarkEmptyStart();  // before the moves below write their flags
    z27MigrateSavedCards();
    ziruSaveChecked = z27MigrateStarterSerials();
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[Z27_SERIAL_MIGRATION_ERR] ") + e.what());
  }
}

// The learned-skill prefs part of a finished move (absolute values, so applying
// it twice is harmless).
void z27ApplyStarterSerialPrefs() {
  JStr plan = loadMeta("z27_starter_serials_prefs");
  if (!plan || plan->empty()) return;
  ObjP keys = Object::parse(*plan);
  auto edit = sp("almanac_state").edit();
  for (const auto& k : keys->keys()) {
    Value v = keys->opt(k);
    if (v.t == Value::STR) edit.putString(k, v.s);
    else edit.remove(k);
  }
  edit.commit();
  ziruDb->deleteMeta("z27_starter_serials_prefs");
}

// The table rows a saved card (id, old serial) can be: same old serial, and the
// id is the listed id or an id the client knows it by. Copy Konami's squad uses first.
std::vector<const ZiruStarterSerial*> z27SerialCandidates(int64_t oldSerial, int64_t id) {
  const ZiruStarters& starters = z27Starters();
  std::vector<const ZiruStarterSerial*> out;
  for (const auto& t : kZiruStarterSerials) {
    if (t.before != oldSerial) continue;
    auto entry = starters.bySerialId.find({t.now, t.id});
    int team = entry != starters.bySerialId.end() ? entry->second->optInt("initial_base_team_id", 0) : 0;
    if (id == t.id || id == z27UpgradeIosCardPid(t.id, team) || id == z27UpgradeIosCardPid(t.id, 0)) out.push_back(&t);
  }
  std::sort(out.begin(), out.end(), [](const ZiruStarterSerial* a, const ZiruStarterSerial* b) { return a->order < b->order; });
  return out;
}

// Returns true when nothing is left to do in this process (moved, nothing to
// move, or waiting for the new data); false after an error (tried again with
// the next request; the transaction left the save as it was).
bool z27MigrateStarterSerials() {
  try {
    z27ApplyStarterSerialPrefs();
    if (loadMeta("z27_starter_serials_v1")) return true;
    const ZiruStarters& starters = z27Starters();
    for (const auto& t : kZiruStarterSerials) {
      if (starters.bySerialId.count({t.now, t.id})) continue;
      if (!ziruSerialWaitingLogged) logRequest("Z27_SERIAL_MIGRATION", "[Z27_SERIAL_MIGRATION_WAITING] the starter list is not the one with a serial per card; the save is left as it is");
      ziruSerialWaitingLogged = true;
      return true;
    }
    std::set<int64_t> oldSerials, newSerials;
    for (const auto& t : kZiruStarterSerials) { oldSerials.insert(t.before); newSerials.insert(t.now); }
    zs::Prefs& prefs = sp("almanac_state");
    auto unwrap = [](const ObjP& o) { return o->optJSONObject("gameplayer") ? o->getJSONObject("gameplayer") : o; };
    auto skillsKey = [](int64_t serial) { return "ziru_skills_" + str(serial); };

    ArrP backupRows = newArr(), backupProgression = newArr(), backupSold = newArr();
    ObjP backupPrefs = newObj(), backupSquads = newObj();
    // A serial's row, careerplan points and learned skills into the backup; false when it has none.
    auto keep = [&](int64_t serial) {
      bool any = false;
      if (JStr row = loadPlayer(serial)) {
        int64_t playerId = 0;
        ziruDb->loadPlayerId(serial, playerId);
        backupRows->put(newObj()->put("serial", (long long)serial)->put("player_id", (long long)playerId)->put("player_json", *row));
        any = true;
      }
      if (JStr points = loadProgression(serial)) {
        backupProgression->put(newObj()->put("serial", (long long)serial)->put("progression_json", *points));
        any = true;
      }
      if (prefs.hasString(skillsKey(serial))) {
        backupPrefs->put(skillsKey(serial), prefs.getString(skillsKey(serial), ""));
        any = true;
      }
      return any;
    };

    bool anything = false;
    std::set<int64_t> clear, soldNow;                         // serials emptied; new serials marked sold
    std::map<int64_t, std::pair<int64_t, std::string>> rows;  // new serial -> (card id, row)
    std::map<int64_t, std::string> points;                    // new serial -> careerplan points
    std::map<std::string, std::string> setPrefs;
    std::set<std::string> removePrefs;
    std::vector<std::string> notes;

    // 1-3. The old serials: the row card moves, everything else there goes.
    for (int64_t old : oldSerials) {
      bool sold = isZiruPlayerSold(old);
      anything |= keep(old) || sold;
      clear.insert(old);
      removePrefs.insert(skillsKey(old));
      if (sold) {
        backupSold->put((long long)old);
        for (const auto& t : kZiruStarterSerials) if (t.before == old) soldNow.insert(t.now);
      }
      JStr text = loadPlayer(old);
      if (!text || text->empty()) continue;
      ObjP row = Object::parse(*text), card = unwrap(row);
      int64_t id = card->getJSONObject("gameplayer_id")->optLong("id", 0), playerId = 0;
      if (ziruDb->loadPlayerId(old, playerId) && playerId != id)
        notes.push_back("serial " + str(old) + ": player_id column " + str(playerId) + " differs from the saved card " + str(id) + "; the card counts");
      if (sold) {
        notes.push_back("card " + str(id) + " under sold serial " + str(old) + " dropped (kept in the backup)");
        continue;
      }
      auto candidates = z27SerialCandidates(old, id);
      if (candidates.empty()) {
        notes.push_back("card " + str(id) + " under " + str(old) + " is no starter card listed there: removed (kept in the backup)");
        continue;
      }
      const ZiruStarterSerial& t = *candidates[0];
      card->getJSONObject("gameplayer_id")->put("serial", (long long)t.now);
      rows[t.now] = {id, row->toString()};
      if (JStr p = loadProgression(old)) points[t.now] = *p;
      if (prefs.hasString(skillsKey(old))) setPrefs[skillsKey(t.now)] = prefs.getString(skillsKey(old), "");
      ArrP learned = card->optJSONArray("additional_skill_id"), boosters = card->optJSONArray("initial_booster_id_list");
      ArrP positions = card->optJSONArray("additional_position");
      notes.push_back("card " + str(id) + " " + str(old) + " -> " + str(t.now) + ": learned " + (learned ? learned->toString() : "[]") +
                      ", boosters " + (boosters ? boosters->toString() : "[]") + ", positions " + (positions ? positions->toString() : "[]") +
                      ", careerplan " + (points.count(t.now) ? points[t.now] : std::string("none")));
    }
    // 4. A new serial that gets no row card keeps a row only when it is that serial's own card.
    for (const auto& t : kZiruStarterSerials) {
      if (oldSerials.count(t.now) || rows.count(t.now)) continue;
      JStr text = loadPlayer(t.now);
      if (text && !text->empty() && z27IsCard(unwrap(Object::parse(*text)), t.id)) continue;
      if (!keep(t.now)) continue;
      anything = true;
      clear.insert(t.now);
      removePrefs.insert(skillsKey(t.now));
      notes.push_back("serial " + str(t.now) + " held state of no card listed there: removed (kept in the backup)");
    }
    // 5. Squad references: (id, old serial) -> new serial.
    auto remap = [&](const ArrP& list, const std::string& where) {
      bool changed = false;
      std::map<std::pair<int64_t, int64_t>, size_t> seen;  // copies of one card already given out in this list
      for (int i = 0; list && i < list->length(); i++) {
        ObjP ref = list->optJSONObject(i);
        ObjP gid = ref ? ref->optJSONObject("gameplayer_id") : nullptr;
        int64_t serial = gid ? gid->optLong("serial", 0) : 0, id = gid ? gid->optLong("id", 0) : 0;
        if (!oldSerials.count(serial)) continue;
        auto candidates = z27SerialCandidates(serial, id);
        size_t copy = candidates.empty() ? 0 : seen[{candidates[0]->id, serial}]++;
        if (copy >= candidates.size()) {
          notes.push_back(where + ": reference " + str(id) + "@" + str(serial) + " matches no starter card listed there; left as it is");
          continue;
        }
        gid->put("serial", (long long)candidates[copy]->now);
        changed = true;
      }
      return changed;
    };
    auto playerList = [](const ObjP& squad) {
      ObjP data = squad->optJSONObject("squad_data") ? squad->getJSONObject("squad_data") : squad;
      return data->optJSONArray("player_list");
    };
    std::map<int32_t, std::string> squads;
    for (auto& [id, text] : ziruDb->loadAllSquadRows()) {
      ObjP squad = Object::parse(text);
      if (!remap(playerList(squad), "squad " + str(id))) continue;
      backupSquads->put(str(id), text);
      squads[id] = squad->toString();
      anything = true;
    }
    JStr special = loadMeta("ziru_myleague_special_squad"), newSpecial;
    if (special && !special->empty()) {
      ObjP squad = Object::parse(*special);
      if (remap(playerList(squad), "MyLeague special squad")) {
        newSpecial = squad->toString();
        anything = true;
      }
    }
    // 6. The squad cached for the active division match is rebuilt at the next match start.
    JStr active = loadMeta("z27_active_match"), newActive;
    if (active && !active->empty()) {
      ObjP match = Object::parse(*active);
      if (match->has("home_squad")) {
        match->remove("home_squad");
        newActive = match->toString();
      }
    }
    if (!anything) {
      ziruDb->saveMeta("z27_starter_serials_v1", "1");
      logRequest("Z27_SERIAL_MIGRATION", "[Z27_SERIAL_MIGRATION] nothing saved under the old starter serials");
      return true;
    }
    ObjP plan = newObj();
    for (const auto& k : removePrefs) plan->put(k, Value::null());
    for (const auto& [k, v] : setPrefs) plan->put(k, v);
    ObjP backup = newObj()->put("rows", backupRows)->put("progression", backupProgression)->put("prefs", backupPrefs)
        ->put("squads", backupSquads)->put("sold_serials", backupSold);
    if (newSpecial) backup->put("myleague_special_squad", *special);
    if (newActive) backup->put("active_match", *active);
    {
      ZiruTx tx(*ziruDb);
      ziruDb->saveMeta("z27_starter_serials_backup", backup->toString());
      for (int64_t serial : clear) ziruDb->deletePlayerState(serial);
      for (const auto& [serial, row] : rows) ziruDb->savePlayer(serial, row.first, row.second);
      for (const auto& [serial, p] : points) ziruDb->saveProgression(serial, p);
      for (const auto& [id, text] : squads) ziruDb->saveSquad(id, text);
      if (newSpecial) ziruDb->saveMeta("ziru_myleague_special_squad", *newSpecial);
      if (newActive) ziruDb->saveMeta("z27_active_match", *newActive);
      for (int64_t old : oldSerials) ziruDb->deleteMeta("sold_player_" + str(old));
      for (int64_t serial : soldNow) ziruDb->saveMeta("sold_player_" + str(serial), "1");
      ziruDb->saveMeta("z27_starter_serials_prefs", plan->toString());
      ziruDb->saveMeta("z27_starter_serials_v1", "1");
      tx.commit();
    }
    z27ApplyStarterSerialPrefs();
    for (const auto& note : notes) logRequest("Z27_SERIAL_MIGRATION", "[Z27_SERIAL_MIGRATION] " + note);
    std::string fresh;
    for (const auto& t : kZiruStarterSerials)
      if (!rows.count(t.now) && !soldNow.count(t.now)) fresh += (fresh.empty() ? "" : " ") + str(t.id) + "@" + str(t.now);
    logRequest("Z27_SERIAL_MIGRATION", "[Z27_SERIAL_MIGRATION] " + str((int64_t)rows.size()) + " cards moved, " + str((int64_t)squads.size()) +
                                           " squads updated; starting with their starter values: " + fresh);
    zs::logi("OFFLINE_SERVER", "[Z27_SERIAL_MIGRATION] save moved to one serial per starter card");
    return true;
  } catch (const std::exception& e) {
    zs::loge("OFFLINE_SERVER", std::string("[Z27_SERIAL_MIGRATION_ERR] ") + e.what());
    logRequest("Z27_SERIAL_MIGRATION", std::string("[Z27_SERIAL_MIGRATION_ERR] ") + e.what() + "; tried again with the next request");
    return false;
  }
}

// ---- A new save starts with no players ----
// A save that holds nothing yet (the first start after a fresh install) is marked once, before the save moves
// (z27MigrateSavedCards, z27MigrateStarterSerials) write their flags. Its club then has none of the starter cards of CmdLogin.json (no starting eleven, no bench, no
// reserves) and an empty squad; the coaches, the formation and every other setting stay. A request naming a starter
// card is refused there (z27TrainingTarget), and the starter cards do not count as the user's cards
// (ziruOwnedCardIds), so the boxes offer them. A save that already holds anything (every save an earlier build has
// opened carries the flag z27MigrateSavedCards writes at its first start) keeps its starter cards as before.
int ziruNoStartersState = -1;  // whether this save has no starter cards: -1 not read yet, 0 no, 1 yes

void z28MarkEmptyStart() {
  if (loadMeta("z28_no_starters")) return;
  if (loadMeta("z27_card_identity_v2") || loadMeta("z27_starter_serials_v1")) return;
  if (!ziruDb->loadAllPlayerRows().empty() || !ziruDb->loadAllSquadRows().empty()) return;
  if (sp("almanac_state").data->length() > 0) return;
  ziruDb->saveMeta("z28_no_starters", "1");
  ziruNoStartersState = 1;
  logRequest("Z28_NO_STARTERS", "[Z28_NO_STARTERS] new save: the club starts with no players");
}

bool z28NoStarters() {
  if (ziruNoStartersState < 0) {
    JStr m = loadMeta("z28_no_starters");
    ziruNoStartersState = m && *m == "1" ? 1 : 0;
  }
  return ziruNoStartersState == 1;
}

// Whether a serial is one a starter card is listed under in CmdLogin.json.
bool z28IsStarterSerial(int64_t serial) { return z27Starters().serials.count(serial) > 0; }
