// The name the mod prints on a player's shirt, taken from the name slots the game keeps for each
// player of a match (ios/ZiruJersey.inc).
//
// Each player in the match data (match + 0x14e0 + side * 0x5dc0 + i * 0x258) holds the name
// slots the game fills from its player database (dt870):
//   kMatchNameOffset   the name shown in the match, as Konami writes it ("van Dijk", "McTominay")
//   kShirtNameOffset   the name printed on the shirt ("VAN DIJK")
//   kOtherNameOffset   one more name slot, read only when the first two give nothing printable
//
// Rules:
//  * The slots are only read. The in-match name stays exactly as the game wrote it: earlier
//    builds rewrote it in Title Case, which turned "van Dijk" into "Van Dijk" and "McTominay"
//    into "Mctominay".
//  * The shirt name is the shirt slot (or the in-match name when the shirt slot is empty) in the
//    capital letters the shirt font has (sanitizeShirtName); failing that, the third slot.
//  * When no slot gives a printable name the answer is empty, and the caller leaves the game's
//    own name on the shirt. There is no table of names by player id: the one earlier builds had
//    gave many players a wrong name.
// Plain C++ so it runs the same in the app and in the host test (test/jerseynametest.cpp).
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace zjn {

constexpr size_t kMatchNameOffset = 0x89;
constexpr size_t kShirtNameOffset = 0xc6;
constexpr size_t kOtherNameOffset = 0x103;
// The slots lie this many bytes apart, so no slot's text is longer than this.
constexpr size_t kNameSlotBytes = kShirtNameOffset - kMatchNameOffset;

// The capitals printed on a shirt: ASCII letters in upper case, spaces, '.', '-' and '\'';
// Latin letters with accents (UTF-8) become a plain capital ("Mbappé" -> "MBAPPE",
// "Çalhanoğlu" -> "CALHANOGLU"); anything else is left out. At most 30 characters.
inline std::string sanitizeShirtName(const char* s) {
  if (!s || !s[0]) return "";
  std::string raw;
  raw.reserve(32);
  for (size_t i = 0; s[i] != '\0' && raw.size() < 30; ++i) {
    unsigned char c = (unsigned char)s[i];
    if (c >= 'a' && c <= 'z') {
      raw += (char)(c - 'a' + 'A');
    } else if ((c >= 'A' && c <= 'Z') || c == ' ' || c == '.' || c == '-' || c == 0x27) {
      raw += (char)c;
    } else if (c == 0xC3 && s[i + 1] != '\0') {
      unsigned char c2 = (unsigned char)s[++i];
      if ((c2 >= 0x80 && c2 <= 0x85) || (c2 >= 0xA0 && c2 <= 0xA5)) raw += 'A';
      else if (c2 == 0x86 || c2 == 0xA6) { raw += 'A'; raw += 'E'; }
      else if (c2 == 0x87 || c2 == 0xA7) raw += 'C';
      else if ((c2 >= 0x88 && c2 <= 0x8B) || (c2 >= 0xA8 && c2 <= 0xAB)) raw += 'E';
      else if ((c2 >= 0x8C && c2 <= 0x8F) || (c2 >= 0xAC && c2 <= 0xAF)) raw += 'I';
      else if (c2 == 0x90 || c2 == 0xB0) raw += 'D';
      else if (c2 == 0x91 || c2 == 0xB1) raw += 'N';
      else if ((c2 >= 0x92 && c2 <= 0x96) || c2 == 0x98 || (c2 >= 0xB2 && c2 <= 0xB6) || c2 == 0xB8) raw += 'O';
      else if ((c2 >= 0x99 && c2 <= 0x9C) || (c2 >= 0xB9 && c2 <= 0xBC)) raw += 'U';
      else if (c2 == 0x9D || c2 == 0xBD || c2 == 0xBF) raw += 'Y';
      else if (c2 == 0x9F) { raw += 'S'; raw += 'S'; }
    } else if (c == 0xC4 && s[i + 1] != '\0') {
      unsigned char c2 = (unsigned char)s[++i];
      if (c2 >= 0x80 && c2 <= 0x85) raw += 'A';
      else if (c2 >= 0x86 && c2 <= 0x8D) raw += 'C';
      else if (c2 >= 0x8E && c2 <= 0x91) raw += 'D';
      else if (c2 >= 0x92 && c2 <= 0x9B) raw += 'E';
      else if (c2 >= 0x9C && c2 <= 0x9F) raw += 'G';
      else if (c2 >= 0xA8 && c2 <= 0xB1) raw += 'I';
    } else if (c == 0xC5 && s[i + 1] != '\0') {
      unsigned char c2 = (unsigned char)s[++i];
      if (c2 >= 0x81 && c2 <= 0x84) raw += 'L';
      else if (c2 >= 0x83 && c2 <= 0x8B) raw += 'N';
      else if (c2 >= 0x8C && c2 <= 0x91) raw += 'O';
      else if (c2 >= 0x94 && c2 <= 0x99) raw += 'R';
      else if ((c2 >= 0x9A && c2 <= 0x9F) || (c2 >= 0xA0 && c2 <= 0xA3)) raw += 'S';
      else if (c2 >= 0xA4 && c2 <= 0xA7) raw += 'T';
      else if (c2 >= 0xAA && c2 <= 0xB3) raw += 'U';
      else if (c2 >= 0xBD && c2 <= 0xC2) raw += 'Z';
    }
  }
  while (!raw.empty() && raw.front() == ' ') raw.erase(raw.begin());
  while (!raw.empty() && raw.back() == ' ') raw.pop_back();
  if (raw.empty() || raw.size() > 30) return "";
  return raw;
}

// The text of one name slot of a player: up to its first NUL, and never past the slot.
inline std::string slotText(const uint8_t* player, size_t offset) {
  const char* s = (const char*)(player + offset);
  return std::string(s, strnlen(s, kNameSlotBytes));
}

// The name to print on the shirt of one player of the match data, or "" when the slots give none.
inline std::string shirtName(const uint8_t* player) {
  if (!player) return "";
  std::string printed = slotText(player, kShirtNameOffset);
  if (printed.empty()) printed = slotText(player, kMatchNameOffset);
  std::string name = sanitizeShirtName(printed.c_str());
  if (name.empty()) name = sanitizeShirtName(slotText(player, kOtherNameOffset).c_str());
  return name;
}

}  // namespace zjn
