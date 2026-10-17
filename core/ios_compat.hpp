#pragma once
#include "jvalue.hpp"

namespace zsv {
// iOS 11.1.1 PlayerBase::SetData accepts 147 parameters. Its indices 142/143
// are skill bits; booster IDs are 144..146 (10 bits each). Ziru 11.0.1 uses
// 142..144 for boosters, including its newer 149-element record templates.
// Keep Android/save/oracle data intact and translate only the iOS wire format.
inline bool adaptIosPlayerParameters(zj::Value& value, bool outgoing) {
  bool changed = false;
  if (value.t == zj::Value::ARR) {
    for (auto& item : value.a->v) changed |= adaptIosPlayerParameters(item, outgoing);
  } else if (value.t == zj::Value::OBJ) {
    for (const auto& key : value.o->keys()) {
      auto item = value.o->get(key);
      if ((key == "parameters" || key == "original_parameters") && item.t == zj::Value::ARR) {
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
        changed |= adaptIosPlayerParameters(item, outgoing);
      }
    }
  }
  return changed;
}
}
