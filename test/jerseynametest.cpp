// Host test for core/jerseyname.hpp: fills fake players of the match data with name slots as
// Konami's dt870 has them and checks the name the mod prints on the shirt, that the name shown in
// the match stays exactly as the game wrote it, and that no name is made up from the player's id.
//   c++ -std=c++17 -Wall -o build/jerseynametest test/jerseynametest.cpp && ./build/jerseynametest
#include "../core/jerseyname.hpp"

#include <cstring>
#include <iostream>
#include <vector>

static int gFails = 0, gOks = 0;
#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (cond) { gOks++; std::cout << "  ok    " << msg << "\n"; }              \
    else { gFails++; std::cout << "  FAIL  " << msg << " (line " << __LINE__ << ")\n"; } \
  } while (0)

// One player of the match data: ios/ZiruJersey.inc steps through a team's players this many bytes apart.
constexpr size_t kPlayerBytes = 0x258;
// Where ios/ZiruJersey.inc reads a player's id.
constexpr size_t kPlayerIdOffset = 0x38;
// The slots lie this many bytes apart (core/jerseyname.hpp).
constexpr size_t kSlotBytes = zjn::kShirtNameOffset - zjn::kMatchNameOffset;

using Player = std::vector<uint8_t>;

static void putSlot(Player& p, size_t offset, const char* text) {
  memcpy(&p[offset], text, strnlen(text, kSlotBytes - 1));  // the byte after the text stays NUL
}
static Player player(uint32_t id, const char* matchName, const char* shirtName, const char* otherName = "") {
  Player p(kPlayerBytes, 0);
  memcpy(&p[kPlayerIdOffset], &id, sizeof id);
  putSlot(p, zjn::kMatchNameOffset, matchName);
  putSlot(p, zjn::kShirtNameOffset, shirtName);
  putSlot(p, zjn::kOtherNameOffset, otherName);
  return p;
}
static std::string slot(const Player& p, size_t offset) {
  return std::string((const char*)&p[offset], strnlen((const char*)&p[offset], kSlotBytes));
}

// The shirt name of a player, and whether reading it left every byte of the player as it was.
struct Read { std::string shirt; bool untouched; };
static Read readShirt(Player& p) {
  Player before = p;
  std::string shirt = zjn::shirtName(p.data());
  return {shirt, p == before};
}

int main() {
  std::cout << "[1] the name shown in the match stays as the game wrote it\n";
  struct Named { const char* match; const char* shirt; const char* printed; };
  const Named named[] = {
      {"van Dijk", "VAN DIJK", "VAN DIJK"},
      {"ter Stegen", "TER STEGEN", "TER STEGEN"},
      {"McTominay", "McTOMINAY", "MCTOMINAY"},
      {"Vini Jr.", "VINI JR.", "VINI JR."},
      {"Kylian Mbappé", "Mbappé", "MBAPPE"},
      {"N'Golo Kanté", "KANTÉ", "KANTE"},
      {"De Bruyne", "DE BRUYNE", "DE BRUYNE"},
  };
  for (const Named& n : named) {
    Player p = player(1, n.match, n.shirt);
    Read r = readShirt(p);
    CHECK(r.untouched && slot(p, zjn::kMatchNameOffset) == n.match,
          "in-match name '" << n.match << "' is left as it is (got '" << slot(p, zjn::kMatchNameOffset) << "')");
    CHECK(r.shirt == n.printed, "shirt slot '" << n.shirt << "' prints '" << n.printed << "' (got '" << r.shirt << "')");
  }

  std::cout << "\n[2] the shirt name is the shirt slot in the shirt font's capitals\n";
  struct Shirt { const char* slot; const char* printed; };
  const Shirt shirts[] = {
      {"MESSI", "MESSI"},
      {"VAN DIJK", "VAN DIJK"},
      {"Mbappé", "MBAPPE"},
      {"Çalhanoğlu", "CALHANOGLU"},
      {"Ødegaard", "ODEGAARD"},
      {"Gündoğan", "GUNDOGAN"},
      {"Szczęsny", "SZCZESNY"},
      {"Müller", "MULLER"},
      {"Rüdiger", "RUDIGER"},
      {"Gavi 6", "GAVI"},       // digits are not in the shirt font
      {"  SON  ", "SON"},       // no spaces at either end
      {"Lewandowski-Kowalczyk-Nowakowski", "LEWANDOWSKI-KOWALCZYK-NOWAKOWS"},  // at most 30 characters
  };
  for (const Shirt& s : shirts) {
    Player p = player(1, "", s.slot);
    CHECK(zjn::shirtName(p.data()) == s.printed, "'" << s.slot << "' -> '" << s.printed << "' (got '" << zjn::shirtName(p.data()) << "')");
    CHECK(zjn::sanitizeShirtName(s.slot) == s.printed, "sanitizeShirtName('" << s.slot << "') == '" << s.printed << "'");
  }
  CHECK(zjn::sanitizeShirtName(nullptr) == "" && zjn::sanitizeShirtName("") == "", "no text gives no name");

  std::cout << "\n[3] which slot is printed\n";
  {
    Player p = player(1, "van Dijk", "");
    Read r = readShirt(p);
    CHECK(r.shirt == "VAN DIJK" && r.untouched && slot(p, zjn::kMatchNameOffset) == "van Dijk",
          "empty shirt slot: the in-match name in capitals, the in-match name itself unchanged");
  }
  {
    Player p = player(1, "久保", "久保 建英", "Kubo");
    Read r = readShirt(p);
    CHECK(r.shirt == "KUBO" && r.untouched, "shirt slot with no letter the font has: the third slot (got '" << r.shirt << "')");
  }
  {
    // The third slot without a NUL, and printable text right after it: only the slot is read.
    Player p = player(1, "", "");
    memset(&p[zjn::kOtherNameOffset], '7', kSlotBytes);
    memcpy(&p[zjn::kOtherNameOffset + kSlotBytes], "ZZZ", 3);
    Read r = readShirt(p);
    CHECK(r.shirt == "" && r.untouched, "a slot is never read past its end (got '" << r.shirt << "')");
  }
  CHECK(zjn::shirtName(nullptr) == "", "no player gives no name");

  std::cout << "\n[4] no name is made up from the player's id\n";
  {
    // 7511 and 4522 are ids the old built-in table named "Messi" and "C. Ronaldo".
    Player empty = player(7511, "", "");
    Read r = readShirt(empty);
    CHECK(r.shirt == "" && r.untouched, "empty name slots give no name, whatever the id (got '" << r.shirt << "')");
    Player foreign = player(4522, "ロナウド", "クリスティアーノ・ロナウド");
    r = readShirt(foreign);
    CHECK(r.shirt == "" && r.untouched,
          "slots with no letter the font has give no name; the game keeps its own (got '" << r.shirt << "')");
  }

  std::cout << "\n" << gOks << " passed, " << gFails << " failed\n";
  return gFails ? 1 : 0;
}
