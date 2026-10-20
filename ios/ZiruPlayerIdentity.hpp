#pragma once
#include <cstdint>
#include <string>

namespace zidentity {
inline uint32_t legendFace(int64_t pid, const std::string& upperName) {
  uint32_t base = (uint32_t)(pid & 0xFFFFFF);
  if (pid > 0) {
    if (base == 9901397 || base == 1397) return 9901397;
    if (base == 37576) return 37576;
    return 0;
  }
  // A name fallback is used only when roster lookup supplied an unambiguous
  // player. Substring matches would also turn Cristiano into Ronaldo R9.
  if (upperName == "ZIDANE" || upperName == "Z. ZIDANE") return 9901397;
  if (upperName == "RONALDO") return 37576;
  return 0;
}
}
