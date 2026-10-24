#pragma once
#include "jvalue.hpp"

namespace zsv {
// Android 11.0.1 Player::GetDataLabel(SkillCard) table, verified in libUE4.so
// at file offset 0xcc12ec. Match by parameter label, not by enum ordinal.
inline constexpr int kZiruSkillParameters[74] = {
  70,107,71,72,73,108,74,75,135,136,102,116,124,130,140,76,121,77,
  120,101,78,103,104,109,127,134,79,80,81,114,128,82,110,83,84,119,
  85,86,98,117,118,122,141,87,88,105,89,90,99,100,138,139,126,129,
  91,92,93,106,111,112,113,125,137,115,94,123,132,95,131,96,97,133,
  143,142
};
inline bool isZiruSkillId(int id) { return id >= 0 && id < 74; }
// The last two skills have no parameter slot in Android's save layout:
// 142..144 are booster IDs there. Keep these skills in the explicit ID lists.
inline bool isZiruSkillParameter(int index) { return index >= 70 && index < 142; }
// Ziru/Android skill numbering. The unnamed "Dummy" skill cards cannot be learned.
inline bool isZiruDummySkill(int id) {
  return id == 9 || id == 14 || id == 42 || id == 50 || id == 51 || id == 72 || id == 73;
}
// Special skills (the client's skill table marks them special).
inline bool isZiruSpecialSkill(int id) {
  switch (id) {
    case 8: case 11: case 12: case 13: case 16: case 18: case 24: case 25: case 29: case 30:
    case 35: case 39: case 40: case 41: case 52: case 53: case 61: case 62: case 63: case 65:
    case 66: case 68: case 71:
      return true;
    default:
      return false;
  }
}
// Skill Training teaches normal skills only.
inline bool isZiruTrainableSkill(int id) { return isZiruSkillId(id) && id < 72 && !isZiruDummySkill(id) && !isZiruSpecialSkill(id); }
// The goalkeeping skills: GK Low Punt, GK High Punt, GK Long Throw, GK Penalty Saver.
inline bool isZiruGoalkeeperSkill(int id) { return id == 44 || id == 45 || id == 47 || id == 49; }
// A goalkeeper card: its registered position, parameters[0], is GK (0).
inline bool isZiruGoalkeeperCard(const zj::ArrP& params) { return params && params->length() > 0 && params->optInt(0, -1) == 0; }
// The skills a card can learn in Skill Training: every normal skill, the goalkeeping
// ones only for a goalkeeper. Konami's answer to CMD_GET_MYCLUB_PLAYER_DATA for an
// outfield card (ziru-extra/responses/get_myclub_player_data.json) lists exactly these
// 40, the card's own skills included.
inline bool isZiruLearnableSkill(int id, bool goalkeeper) {
  return isZiruTrainableSkill(id) && (goalkeeper || !isZiruGoalkeeperSkill(id));
}
// CMD_GET_MYCLUB_PLAYER_DATA skillcard_id_list: the skills the card can learn, not the
// skills it has. The iOS client stores this list in ParameterMyClubPlayerData+0x2a0
// (CommandObjectCmdGetMyclubPlayerData::QuitCommand, 0x104769554) and shows only these
// skills in Skill Training (Select) and (Category) (UtilityMyClubTraining::
// GetCategorySkillIdList, 0x1048a5a14) and in Available Skills. It greys out the ones the
// card already has by itself (UtilityMyClubTraining::HasSkill, 0x1048a5aec). A list of
// the card's own skills therefore greys out every category as "(learned)".
inline zj::ArrP ziruLearnableSkillList(const zj::ArrP& params) {
  auto result = zj::Array::make();
  bool goalkeeper = isZiruGoalkeeperCard(params);
  for (int id = 0; id < 72; ++id) if (isZiruLearnableSkill(id, goalkeeper)) result->put(id);
  return result;
}
// A learned skill read back from a save, or -1 when it must be dropped.
// Older builds stored a learned Snap Strike (25) or Attack Trigger (68) as 72/73,
// the iOS numbers of two Dummy cards; those are mapped back. Dummy skills and
// unknown ids are dropped. Other learned skills, special ones included, are the
// user's progress and are kept.
inline int ziruRepairLearnedSkill(int id) {
  if (id == 72) return 25;
  if (id == 73) return 68;
  if (!isZiruSkillId(id) || isZiruDummySkill(id)) return -1;
  return id;
}
inline zj::ArrP ziruSkillList(const zj::ArrP& params) {
  auto result = zj::Array::make();
  if (!params) return result;
  for (int id = 0; id < 72; ++id) {
    int index = kZiruSkillParameters[id];
    if (index < params->length() && params->optInt(index, 0) == 1) result->put(id);
  }
  return result;
}
inline int iosSkillId(int id, bool outgoing) {
  if (outgoing) {
    if (id == 72) return 25;
    if (id == 73) return 68;
    if (!isZiruSkillId(id)) return std::max(0, std::min(73, id));
    return id + (id >= 25) + (id >= 67);
  }
  // iOS wire format -> internal format
  if (id < 0) return 0;
  if (id == 25) return 72;
  if (id == 68) return 73;
  if (id >= 74) return 71;
  return id - (id > 25) - (id > 68);
}
// iOS 11.1.1 PlayerBase::SetData accepts 147 parameters. Its indices 142/143
// are skill bits; booster IDs are 144..146 (10 bits each). Ziru 11.0.1 uses
// 142..144 for boosters, including its newer 149-element record templates.
// Keep Android/save/oracle data intact and translate only the iOS wire format.
inline bool adaptIosPlayerData(zj::Value& value, bool outgoing) {
  bool changed = false;
  if (value.t == zj::Value::ARR) {
    for (auto& item : value.a->v) changed |= adaptIosPlayerData(item, outgoing);
  } else if (value.t == zj::Value::OBJ) {
    // Player-detail menus also read the owned-player object from entry_info.
    // Emit the explicit skill list there, not only in START_GAME responses.
    auto playerParams = value.o->optJSONArray("parameters");
    if (outgoing && value.o->optJSONObject("gameplayer_id") && playerParams && playerParams->length() >= 142) {
      value.o->put("skillcard_id_list", ziruSkillList(playerParams));
      changed = true;
    }
    // Verified against dt870 Coach.bin (176-byte records) and native
    // CoachBase::SetWorkFromPesdb at 0x104b93814: ability_id[3] is the
    // 10-bit field at bit 10 of the qword at record+16. Two injected coach
    // templates had this field shifted once too far (12->24, 14->28).
    // Repair on the wire too, because older squad saves retain the templates.
    auto cid = value.o->optJSONObject("coach_id");
    auto abilities = value.o->optJSONArray("ability_id");
    if (outgoing && cid && abilities && abilities->length() == 4) {
      int64_t id = cid->optLong("id", 0);
      if (id == 17609097478250LL && abilities->optInt(3, 0) == 24) { abilities->put(3, 12); changed = true; }
      if (id == 17608560707469LL && abilities->optInt(3, 0) == 28) { abilities->put(3, 14); changed = true; }
    }
    for (const auto& key : value.o->keys()) {
      auto item = value.o->get(key);
      if ((key == "additional_skill_id" || key == "skillcard_id_list" || key == "select_skill_list") && item.t == zj::Value::ARR) {
        auto skills = zj::Array::make();
        for (int i = 0; i < item.a->length(); ++i) skills->put(iosSkillId(item.a->getInt(i), outgoing));
        value.o->put(key, skills);
        changed = true;
      } else if ((key == "additional_skill_id" || key == "skill_id") && (item.t == zj::Value::INT || item.t == zj::Value::LONG)) {
        value.o->put(key, iosSkillId((int)item.n, outgoing));
        changed = true;
      } else if ((key == "parameters" || key == "original_parameters") && item.t == zj::Value::ARR) {
        auto src = item.a;
        int n = src->length();
        if ((outgoing && (n == 145 || n == 149)) || (!outgoing && n == 147)) {
          auto dst = zj::Array::make();
          for (int i = 0; i < 142; ++i) dst->put(src->get(i));
          // iOS 142/143 are the flags of the two Dummy skill cards (iOS 68/25). No card has
          // them and they cannot be learned; a learned Snap Strike or Attack Trigger has its own flag.
          if (outgoing) dst->put(0)->put(0);
          for (int i = 0; i < 3; ++i) dst->put(src->get((outgoing ? 142 : 144) + i));
          value.o->put(key, dst);
          changed = true;
        }
      } else {
        changed |= adaptIosPlayerData(item, outgoing);
      }
    }
  }
  return changed;
}
}
