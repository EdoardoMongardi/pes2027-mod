// Host test for the jersey hook itself (ios/ZiruJersey.inc). The hook is compiled here as
// Objective-C++ against stand-ins for the iOS declarations it uses. A fake match sits where the hook
// looks for it, and the hook's own shirt-name lookups run on it. The test checks that the lookups
// find the right player's name and leave every byte of the match data as it was, so the names shown
// in the match stay as the game wrote them.
//   clang++ -std=c++17 -x objective-c++ -fobjc-runtime=ios-15.0 -Wall -Wno-unused -o build/jerseyhooktest test/jerseyhooktest.mm
// The hook's Objective-C code (logging, the name-atlas hooks) and its calls into the game that set the
// shirt are never run here; tmpdb::CommonWork::GetPlayer is left out by giving the hook no CommonWork.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

// Stand-ins for the Foundation, dyld and Mach declarations the hook names.
__attribute__((objc_root_class)) @interface NSObject
+ (instancetype)alloc;
@end
@interface NSString : NSObject
- (instancetype)initWithBytes:(const void*)bytes length:(unsigned long)length encoding:(unsigned long)encoding;
- (instancetype)initWithCharacters:(const unsigned short*)characters length:(unsigned long)length;
- (unsigned long)length;
@end
@interface NSConstantString : NSString
@end
typedef unsigned long NSUInteger;
typedef unsigned short unichar;
enum { NSUTF8StringEncoding = 4 };
static void ZLog(NSString* format, ...) {}
#if __has_include(<mach/mach.h>)
#include <mach-o/dyld.h>
#include <mach/mach.h>
#else
struct mach_header { uint32_t magic, cputype, cpusubtype, filetype; };
enum { MH_EXECUTE = 2 };
uint32_t _dyld_image_count();
const mach_header* _dyld_get_image_header(uint32_t image);
intptr_t _dyld_get_image_vmaddr_slide(uint32_t image);
typedef uintptr_t vm_address_t;
typedef uintptr_t vm_size_t;
typedef int kern_return_t;
extern vm_size_t vm_page_size;
enum { VM_PROT_READ = 1, VM_PROT_WRITE = 2, VM_PROT_COPY = 0x10, KERN_SUCCESS = 0 };
unsigned mach_task_self();
kern_return_t vm_protect(unsigned task, vm_address_t address, vm_size_t size, int setMaximum, int protection);
#endif
// The team ids the server sets for the match (ios/ZiruOffline.mm); this test gives each player's side directly.
extern "C" int32_t gZiruHomeTeamId = 0;
extern "C" int32_t gZiruAwayTeamId = 0;

#include "../core/jerseyname.hpp"  // the name slots
#include "../ios/ZiruJersey.inc"

static int gFails = 0, gOks = 0;
#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (cond) { gOks++; std::cout << "  ok    " << msg << "\n"; }              \
    else { gFails++; std::cout << "  FAIL  " << msg << " (line " << __LINE__ << ")\n"; } \
  } while (0)

// Where ios/ZiruJersey.inc finds the match: it calls tmpdb::Manager::GetInstance at this game address
// (plus the slide), reads the manager's data pointer, and the match lies this far into that data.
constexpr uintptr_t kGetInstance = 0x104bba084;
constexpr size_t kManagerData = 0x48;
constexpr size_t kManagerCommonWork = 0x40;  // left null: the hook then does not call GetPlayer
constexpr size_t kManagerBytes = 0x50;
constexpr size_t kMatchInData = 0x6c4c0;
// A side's players in the match, and one player.
constexpr size_t kTeamPlayers = 0x14e0;
constexpr size_t kSideBytes = 0x5dc0;
constexpr size_t kPlayerBytes = 0x258;
constexpr size_t kMatchBytes = kTeamPlayers + 2 * kSideBytes;
// Fields of a player the hook reads besides the name slots.
constexpr size_t kPlayerId = 0x38;
constexpr size_t kUniformNumber = 0x1ec;
// The side flag of a player actor (0 home, 1 away) and an actor big enough for every field the hook reads.
constexpr size_t kActorSide = 0x1110;
constexpr size_t kActorBytes = 0x2000;
// The slots lie this many bytes apart (core/jerseyname.hpp).
constexpr size_t kSlotBytes = zjn::kShirtNameOffset - zjn::kMatchNameOffset;

static std::vector<uint8_t> gData(kMatchInData + kMatchBytes, 0);
static uint8_t gManager[kManagerBytes];
static void* fakeGetInstance() { return gManager; }

static uint8_t* match() { return gData.data() + kMatchInData; }
static uint8_t* player(int side, int index) { return match() + kTeamPlayers + side * kSideBytes + index * kPlayerBytes; }
static void putSlot(uint8_t* p, size_t offset, const char* text) {
  memcpy(p + offset, text, strnlen(text, kSlotBytes - 1));  // the byte after the text stays NUL
}
static void putPlayer(int side, int index, uint32_t id, uint16_t number, const char* matchName, const char* shirtName) {
  uint8_t* p = player(side, index);
  memcpy(p + kPlayerId, &id, sizeof id);
  memcpy(p + kUniformNumber, &number, sizeof number);
  putSlot(p, zjn::kMatchNameOffset, matchName);
  putSlot(p, zjn::kShirtNameOffset, shirtName);
}
static std::string matchName(int side, int index) {
  const char* s = (const char*)player(side, index) + zjn::kMatchNameOffset;
  return std::string(s, strnlen(s, kSlotBytes));
}
static std::vector<uint8_t> actorOn(int32_t side) {
  std::vector<uint8_t> actor(kActorBytes, 0);
  memcpy(actor.data() + kActorSide, &side, sizeof side);
  return actor;
}

int main() {
  *(uint8_t**)(gManager + kManagerData) = gData.data();
  *(void**)(gManager + kManagerCommonWork) = nullptr;
  // The hook calls GetInstance at kGetInstance + gSlide; this slide makes that the fake one.
  zjersey::gSlide = (intptr_t)((uintptr_t)&fakeGetInstance - kGetInstance);

  // Name slots as Konami's dt870 has them. Both sides have a number 4.
  putPlayer(0, 0, 41571, 1, "ter Stegen", "TER STEGEN");
  putPlayer(0, 3, 45843, 4, "van Dijk", "VAN DIJK");
  putPlayer(0, 6, 133001, 7, "McTominay", "McTOMINAY");
  // 7511 and 4522 are ids the old built-in name table named "Messi" and "C. Ronaldo".
  putPlayer(0, 9, 7511, 10, "", "");
  putPlayer(0, 10, 4522, 11, "ロナウド", "クリスティアーノ・ロナウド");
  putPlayer(1, 3, 110718, 4, "Kylian Mbappé", "Mbappé");
  putPlayer(1, 15, 117047, 20, "Vini Jr.", "VINI JR.");  // on the bench
  const std::vector<uint8_t> before(match(), match() + kMatchBytes);

  std::vector<uint8_t> home = actorOn(0), away = actorOn(1), unknown = actorOn(-1);

  std::cout << "[1] the shirt name of the player with the actor's number, on the actor's side\n";
  struct ByNumber { std::vector<uint8_t>* actor; const char* side; int number; const char* printed; };
  const ByNumber byNumber[] = {
      {&home, "home", 4, "VAN DIJK"},
      {&away, "away", 4, "MBAPPE"},
      {&home, "home", 1, "TER STEGEN"},
      {&home, "home", 7, "MCTOMINAY"},
      {&away, "away", 20, "VINI JR."},
      {&unknown, "unknown side (home first)", 4, "VAN DIJK"},
      {&home, "home", 20, ""},  // number 20 plays for the other side
  };
  for (const ByNumber& c : byNumber) {
    std::string got = zjersey::lookupShirtName(c.actor->data(), c.number);
    CHECK(got == c.printed, c.side << " #" << c.number << " -> '" << c.printed << "' (got '" << got << "')");
  }

  std::cout << "\n[2] the shirt name of the player with the actor's player id\n";
  {
    std::string got = zjersey::lookupShirtNameByPlayerId(unknown.data(), 110718);
    CHECK(got == "MBAPPE", "player 110718 -> 'MBAPPE' (got '" << got << "')");
    got = zjersey::lookupPlayerNameUniversal(home.data(), 45843, 0);
    CHECK(got == "VAN DIJK", "player 45843 with no number -> 'VAN DIJK' (got '" << got << "')");
  }

  std::cout << "\n[3] no name is made up from the player's id\n";
  {
    std::string got = zjersey::lookupPlayerNameUniversal(home.data(), 7511, 10);
    CHECK(got == "", "player 7511 with empty name slots gets no name; the game keeps its own (got '" << got << "')");
    got = zjersey::lookupPlayerNameUniversal(home.data(), 4522, 11);
    CHECK(got == "", "player 4522 with no letter the font has gets no name (got '" << got << "')");
  }

  std::cout << "\n[4] the lookups leave the match data as the game wrote it\n";
  struct Shown { int side, index; const char* name; };
  const Shown shown[] = {{0, 0, "ter Stegen"}, {0, 3, "van Dijk"}, {0, 6, "McTominay"}, {1, 3, "Kylian Mbappé"}, {1, 15, "Vini Jr."}};
  for (const Shown& s : shown)
    CHECK(matchName(s.side, s.index) == s.name, "in-match name '" << s.name << "' (got '" << matchName(s.side, s.index) << "')");
  CHECK(std::vector<uint8_t>(match(), match() + kMatchBytes) == before, "every byte of the match data is unchanged");

  std::cout << "\n" << gOks << " passed, " << gFails << " failed\n";
  return gFails ? 1 : 0;
}
