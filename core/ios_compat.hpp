#pragma once
#include "jvalue.hpp"

namespace zsv {
// Android 11.0.1 Player::GetDataLabel(SkillCard) table, verified in libUE4.so
// at file offset 0xcc12ec. Match by parameter label, not by enum ordinal.
inline constexpr int kZiruSkillParameters[72] = {
  70,107,71,72,73,108,74,75,135,136,102,116,124,130,140,76,121,77,
  120,101,78,103,104,109,127,134,79,80,81,114,128,82,110,83,84,119,
  85,86,98,117,118,122,141,87,88,105,89,90,99,100,138,139,126,129,
  91,92,93,106,111,112,113,125,137,115,94,123,132,95,131,96,97,133
};
inline bool isZiruSkillId(int id) { return id >= 0 && id < 72; }
inline int iosSkillId(int id, bool outgoing) {
  if (outgoing) {
    if (!isZiruSkillId(id)) throw std::invalid_argument("Unsupported Ziru skill ID");
    return id + (id >= 25) + (id >= 67);
  }
  // iOS adds two skills at enum positions 25 and 68, not at the end.
  // They are not offered in Ziru's training list; do not silently give another skill.
  if (id < 0 || id >= 74 || id == 25 || id == 68)
    throw std::invalid_argument("Skill not supported by the Ziru save format");
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
    for (const auto& key : value.o->keys()) {
      auto item = value.o->get(key);
      if ((key == "additional_skill_id" || key == "skillcard_id_list") && item.t == zj::Value::ARR) {
        auto skills = zj::Array::make();
        for (int i = 0; i < item.a->length(); ++i) skills->put(iosSkillId(item.a->getInt(i), outgoing));
        value.o->put(key, skills);
        changed = true;
      } else if (key == "additional_skill_id" && (item.t == zj::Value::INT || item.t == zj::Value::LONG)) {
        value.o->put(key, iosSkillId((int)item.n, outgoing));
        changed = true;
      } else if ((key == "parameters" || key == "original_parameters") && item.t == zj::Value::ARR) {
        auto src = item.a;
        int n = src->length();
        if ((outgoing && (n == 145 || n == 149)) || (!outgoing && n == 147)) {
          auto dst = zj::Array::make();
          for (int i = 0; i < 142; ++i) dst->put(src->get(i));
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
