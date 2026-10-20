#include "../ios/ZiruPlayerIdentity.hpp"
#include <cassert>

int main() {
  using zidentity::legendFace;
  assert(legendFace(37576, "RONALDO") == 37576);
  assert(legendFace(9901397, "ZIDANE") == 9901397);
  assert(legendFace((1LL << 32) | 37576, "") == 37576);
  assert(legendFace(4522, "RONALDO") == 0);
  assert(legendFace(0, "C. RONALDO") == 0);
  assert(legendFace(0, "CRISTIANO RONALDO") == 0);
  assert(legendFace(0, "RONALDO") == 37576);
  assert(legendFace(0, "Z. ZIDANE") == 9901397);
  assert(legendFace(0, "ZIDANE JR") == 0);
}
