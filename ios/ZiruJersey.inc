// Native name-atlas fallback, PESmobile 11.1.101 only. These data vtable slots
// are validated before writing; no executable page is modified at runtime.
// The shirt name comes from the player's name slots in the match data (core/jerseyname.hpp);
// the match data itself is only read, so the names shown in the match stay as the game wrote them.
#include "../core/jerseyname.hpp"

namespace zjersey {
using LoadFn = void (*)(void*);
using ReadyFn = bool (*)(void*);
using DrawFn = void (*)(void*, void*);
struct NativeString { char16_t* data; int32_t num; int32_t max; };
using AssetFn = void (*)(void*, NativeString*, bool);
using SetUniformNumberFn = void (*)(void* actor, int uniformNumber);
using SetupFromPlayerIdFn = void (*)(void* actor, int64_t playerId, void* path, int uniformIndex, void* uniName);
using BackNumberStepFn = void (*)(void* actor);

static LoadFn originalLoad;
static ReadyFn originalReady;
static DrawFn originalDraw;
static AssetFn originalAsset;
static SetUniformNumberFn originalSetUniformNumber;
static SetupFromPlayerIdFn originalSetupFromPlayerId;
static BackNumberStepFn originalBackNumberStep;

static std::mutex mutex;
static std::set<void*> retried, recorded;
static int logBudget = 96;
static intptr_t gSlide = 0;

static NSString* nameText(void* actor) {
  const uint8_t* s = (const uint8_t*)actor + 0x278;
  bool heap = (int8_t)s[23] < 0;
  uint64_t length = heap ? ((const uint64_t*)s)[1] : s[23];
  const char* text = heap ? *(const char**)s : (const char*)s;
  if (!text || length > 120) return @"<invalid>";
  return [[NSString alloc] initWithBytes:text length:(NSUInteger)length encoding:NSUTF8StringEncoding] ?: @"<invalid UTF-8>";
}
static NSString* assetText(void* actor) {
  auto s = (NativeString*)((uint8_t*)actor + 0x2c0);
  if (!s->data || s->num < 2 || s->num > 512) return @"<empty>";
  return [[NSString alloc] initWithCharacters:(const unichar*)s->data length:s->num - 1];
}
static void load(void* actor) {
  {
    std::lock_guard<std::mutex> guard(mutex);
    retried.erase(actor); recorded.erase(actor);
    if (logBudget > 0) {
      --logBudget;
      ZLog(@"[JERSEY] request name='%@' atlas=%@ scale=%.4f", nameText(actor), assetText(actor), *(float*)((uint8_t*)actor + 0x298));
    }
  }
  originalLoad(actor);
}
static bool ready(void* actor) {
  if (actor) *(uint8_t*)((uint8_t*)actor + 0x290) = 0;
  if (!originalReady(actor)) return false;
  auto p = (uint8_t*)actor;
  // A completed async load with a null primary atlas produces a blank canvas.
  // Retry once with the stock iOS atlas verified in pc0103's directory index.
  // Existing and custom successfully loaded atlases always remain in use.
  bool retry = false;
  if (!*(void**)(p + 0x2b0) && nameText(actor).length > 0) {
    std::lock_guard<std::mutex> guard(mutex);
    if (retried.insert(actor).second) retry = true;
  }
  if (retry) {
    ZLog(@"[JERSEY] missing atlas %@; retry stock iOS name atlas", assetText(actor));
    static char16_t fallback[] = u"T_EditUni_FontName_00_D";
    NativeString s{fallback, (int32_t)(sizeof(fallback) / sizeof(fallback[0])), (int32_t)(sizeof(fallback) / sizeof(fallback[0]))};
    originalAsset(actor, &s, true);
    originalLoad(actor);
    return false;
  }
  return true;
}
static void draw(void* actor, void* canvas) {
  {
    std::lock_guard<std::mutex> guard(mutex);
    if (recorded.insert(actor).second && logBudget > 0) {
      --logBudget;
      ZLog(@"[JERSEY] draw name='%@' atlas=%@ texture=%p scale=%.4f", nameText(actor), assetText(actor),
          *(void**)((uint8_t*)actor + 0x2b0), *(float*)((uint8_t*)actor + 0x298));
    }
  }
  if (actor) *(uint8_t*)((uint8_t*)actor + 0x290) = 0;
  originalDraw(actor, canvas);
}

extern "C" int32_t gZiruHomeTeamId;
extern "C" int32_t gZiruAwayTeamId;

static std::mutex sActorLock;
static std::unordered_map<void*, int64_t> sActorPlayerId;

static int resolveActorSide(void* actor, uint8_t* match) {
  if (!actor) return -1;

  // 1. Check actor + 0x1110 (native side flag: 0 = Home, 1 = Away)
  int32_t actorSide = *(int32_t*)((uint8_t*)actor + 0x1110);
  if (actorSide == 0 || actorSide == 1) return actorSide;

  // 2. Check actor + 0x450 uniform path string (e.g. u0108p1, u0128p2, u0108g1, u0128g2)
  const char16_t* uStr = *(const char16_t**)((uint8_t*)actor + 0x450);
  int32_t uLen = *(int32_t*)((uint8_t*)actor + 0x458);
  if (uStr && uLen >= 6) {
    // Check kit slot indicator ('p1' or 'g1' = home, 'p2' or 'g2' = away)
    for (int i = 0; i < uLen - 1; ++i) {
      if ((uStr[i] == u'p' || uStr[i] == u'g') && uStr[i+1] == u'1') return 0;
      if ((uStr[i] == u'p' || uStr[i] == u'g') && uStr[i+1] == u'2') return 1;
    }
    // Check 4-digit team ID after 'u' or 'g'
    if ((uStr[0] == u'u' || uStr[0] == u'g') && uLen >= 5) {
      int32_t kitTeamId = 0;
      for (int i = 1; i <= 4 && i < uLen; ++i) {
        if (uStr[i] >= u'0' && uStr[i] <= u'9') {
          kitTeamId = kitTeamId * 10 + (uStr[i] - u'0');
        }
      }
      if (kitTeamId > 0 && match) {
        uint32_t homeTeamId = *(uint32_t*)(match + 0x11d0 + 0x10);
        uint32_t awayTeamId = *(uint32_t*)(match + 0x11d0 + 0x188 + 0x10);
        if (homeTeamId == 0 && gZiruHomeTeamId > 0) homeTeamId = (uint32_t)gZiruHomeTeamId;
        if (awayTeamId == 0 && gZiruAwayTeamId > 0) awayTeamId = (uint32_t)gZiruAwayTeamId;
        if ((uint32_t)kitTeamId == homeTeamId) return 0;
        if ((uint32_t)kitTeamId == awayTeamId) return 1;
      }
    }
  }

  // 3. Check actor + 0xe10 (teamId)
  int32_t actorTeamId = *(int32_t*)((uint8_t*)actor + 0xe10);
  if (actorTeamId > 0 && match) {
    uint32_t homeTeamId = *(uint32_t*)(match + 0x11d0 + 0x10);
    uint32_t awayTeamId = *(uint32_t*)(match + 0x11d0 + 0x188 + 0x10);
    if (homeTeamId == 0 && gZiruHomeTeamId > 0) homeTeamId = (uint32_t)gZiruHomeTeamId;
    if (awayTeamId == 0 && gZiruAwayTeamId > 0) awayTeamId = (uint32_t)gZiruAwayTeamId;
    if ((uint32_t)actorTeamId == homeTeamId) return 0;
    if ((uint32_t)actorTeamId == awayTeamId) return 1;
  }

  return -1;
}

static std::string lookupShirtName(void* actor, int uniformNumber) {
  if (uniformNumber <= 0 || uniformNumber > 99) return "";

  auto getMgr = (void* (*)())(0x104bba084 + gSlide);
  void* mgr = getMgr ? getMgr() : nullptr;
  if (!mgr) return "";

  uint8_t* base = *(uint8_t**)((uint8_t*)mgr + 0x48);
  if (!base) return "";

  uint8_t* match = base + 0x6c4c0;

  int preferredSide = resolveActorSide(actor, match);

  auto checkSide = [&](int side) -> std::string {
    if (side < 0 || side > 1) return "";
    uint8_t* teamPlayers = match + 0x14e0 + (side * 0x5dc0);
    uint8_t* teamObj = match + 0x620 + (side * 0x5d8);
    uint16_t* uniformNums = (uint16_t*)(teamObj + 0x110);

    // Pass 1: Starters (0..10)
    for (int i = 0; i < 11; ++i) {
      uint8_t* p = teamPlayers + (i * 0x258);
      uint16_t num1 = *(uint16_t*)(p + 0x1ec);
      uint16_t num2 = *(uint16_t*)(p + 0x1b4);
      uint16_t num3 = uniformNums[i];
      if (num1 == (uint16_t)uniformNumber || num2 == (uint16_t)uniformNumber || num3 == (uint16_t)uniformNumber) {
        std::string name = zjn::shirtName(p);
        if (!name.empty()) return name;
      }
    }

    // Pass 2: Bench (11..39)
    for (int i = 11; i < 40; ++i) {
      uint8_t* p = teamPlayers + (i * 0x258);
      uint16_t num1 = *(uint16_t*)(p + 0x1ec);
      uint16_t num2 = *(uint16_t*)(p + 0x1b4);
      uint16_t num3 = uniformNums[i];
      if (num1 == (uint16_t)uniformNumber || num2 == (uint16_t)uniformNumber || num3 == (uint16_t)uniformNumber) {
        std::string name = zjn::shirtName(p);
        if (!name.empty()) return name;
      }
    }
    return "";
  };

  if (preferredSide >= 0) {
    return checkSide(preferredSide);
  }
  std::string res = checkSide(0);
  if (!res.empty()) return res;
  return checkSide(1);
}

static std::string lookupShirtNameByPlayerId(void* actor, int64_t playerId) {
  if (playerId <= 0) return "";
  uint32_t targetBaseId = (uint32_t)(playerId & 0xFFFFFF);

  auto getMgr = (void* (*)())(0x104bba084 + gSlide);
  void* mgr = getMgr ? getMgr() : nullptr;
  if (!mgr) return "";

  uint8_t* base = *(uint8_t**)((uint8_t*)mgr + 0x48);
  if (!base) return "";

  uint8_t* match = base + 0x6c4c0;

  int preferredSide = resolveActorSide(actor, match);

  auto checkSide = [&](int side) -> std::string {
    if (side < 0 || side > 1) return "";
    uint8_t* teamPlayers = match + 0x14e0 + (side * 0x5dc0);
    for (int i = 0; i < 40; ++i) {
      uint8_t* p = teamPlayers + (i * 0x258);
      uint32_t baseId = *(uint32_t*)(p + 0x38);
      uint64_t fullPid = *(uint64_t*)(p + 0x38);
      if ((baseId & 0xFFFFFF) == targetBaseId || (fullPid & 0xFFFFFF) == (uint64_t)targetBaseId || fullPid == (uint64_t)playerId) {
        std::string name = zjn::shirtName(p);
        if (!name.empty()) return name;
      }
    }
    return "";
  };

  if (preferredSide >= 0) {
    return checkSide(preferredSide);
  }
  std::string res = checkSide(0);
  if (!res.empty()) return res;
  return checkSide(1);
}

static std::string lookupPlayerNameUniversal(void* actor, int64_t playerId, int uniformNumber) {
  // 1. Try match roster lookup by uniform number (most accurate for on-field actor)
  if (uniformNumber > 0 && uniformNumber <= 99) {
    std::string n = lookupShirtName(actor, uniformNumber);
    if (!n.empty()) return n;
  }

  // 2. Try match roster lookup by player ID
  if (playerId > 0) {
    std::string n = lookupShirtNameByPlayerId(actor, playerId);
    if (!n.empty()) return n;
  }

  // 3. Try the game's own player record: tmpdb::CommonWork::GetPlayer with playerId
  if (playerId > 0 || (playerId & 0xFFFFFF) > 0) {
    auto getMgr = (void* (*)())(0x104bba084 + gSlide);
    void* mgr = getMgr ? getMgr() : nullptr;
    if (mgr) {
      void* pmgr = *(void**)((uint8_t*)mgr + 0x40);
      if (pmgr) {
        auto findPl = (void* (*)(void*, int64_t, int64_t))(0x104b9588c + gSlide);
        if (findPl) {
          void* pl = nullptr;
          if (playerId > 0) {
            pl = findPl(pmgr, 0, playerId);
            if (!pl && (playerId & 0xFFFFFF) > 0) {
              pl = findPl(pmgr, (int64_t)(playerId & 0xFFFFFF), 0);
            }
            if (!pl && (uint32_t)playerId > 0) {
              pl = findPl(pmgr, (int64_t)(uint32_t)playerId, 0);
            }
          }
          if (pl) {
            // A tmpdb::Player has the same name slots as a player of the match data (core/jerseyname.hpp).
            std::string n = zjn::shirtName((const uint8_t*)pl);
            if (!n.empty()) return n;
          }
        }
      }
    }
  }

  // No name found: the callers keep the shirt name the game set (BackNumberStep only puts it in capitals).
  return "";
}

static void applyActorShirtName(void* actor, const std::string& name) {
  if (!actor || name.empty()) return;
  auto setShirt = (void (*)(void*, const char*))(0x10090ef34 + gSlide);
  if (setShirt) {
    setShirt(actor, name.c_str());
  }
  *(uint8_t*)((uint8_t*)actor + 0xf10) = 1;
  *(uint8_t*)((uint8_t*)actor + 0x1aaf) = 1;
  // NOTE: Never overwrite actor + 0xe38!
  // If set to 1, triggers referee branch at 0x1008ecb88 and aborts uniform creation (black screen).
  // If set to 0, triggers cbz w23 at 0x1008a0184, which skips font color setup.
  // Leave actor + 0xe38 at its native value.

  void* nameTexActor = *(void**)((uint8_t*)actor + 0x1918);
  if (nameTexActor) {
    auto setNameFn = (void (*)(void*, void*))(0x100851098 + gSlide);
    if (setNameFn) setNameFn(nameTexActor, (uint8_t*)actor + 0x1710);
    *(uint8_t*)((uint8_t*)nameTexActor + 0x290) = 0; // 0 = render player name; 1 = debug font test grid (orange box)
  }
}

static void hookedSetUniformNumber(void* actor, int uniformNumber) {
  if (originalSetUniformNumber) {
    originalSetUniformNumber(actor, uniformNumber);
  }
  if (!actor || uniformNumber <= 0) return;
  int64_t pid = 0;
  {
    std::lock_guard<std::mutex> lk(sActorLock);
    auto it = sActorPlayerId.find(actor);
    if (it != sActorPlayerId.end()) pid = it->second;
  }
  std::string name = lookupPlayerNameUniversal(actor, pid, uniformNumber);
  if (!name.empty()) {
    applyActorShirtName(actor, name);
    ZLog(@"[JERSEY] SetUniformNumber #%d pid=%lld -> '%s'", uniformNumber, (long long)pid, name.c_str());
  }
}

static void hookedSetupFromPlayerId(void* actor, int64_t playerId, void* path, int uniformIndex, void* uniName) {
  if (originalSetupFromPlayerId) {
    originalSetupFromPlayerId(actor, playerId, path, uniformIndex, uniName);
  }
  if (!actor) return;
  {
    std::lock_guard<std::mutex> lk(sActorLock);
    if (sActorPlayerId.size() > 512) sActorPlayerId.clear();
    sActorPlayerId[actor] = playerId;
  }
  int32_t num = *(int32_t*)((uint8_t*)actor + 0x1708);
  std::string name = lookupPlayerNameUniversal(actor, playerId, num);
  if (!name.empty()) {
    applyActorShirtName(actor, name);
    ZLog(@"[JERSEY] SetupFromPlayerId pid=%lld num=%d -> '%s'", (long long)playerId, num, name.c_str());
  }
}

static void hookedBackNumberStep(void* actor) {
  if (actor) {
    int32_t num = *(int32_t*)((uint8_t*)actor + 0x1708);
    if (num <= 0 || num > 99) {
      int32_t altNum = *(int32_t*)((uint8_t*)actor + 0x1924);
      if (altNum > 0 && altNum <= 99) num = altNum;
    }
    int64_t pid = 0;
    {
      std::lock_guard<std::mutex> lk(sActorLock);
      auto it = sActorPlayerId.find(actor);
      if (it != sActorPlayerId.end()) pid = it->second;
    }

    const uint8_t* s = (const uint8_t*)actor + 0x1710;
    bool heap = (int8_t)s[23] < 0;
    uint64_t len = heap ? ((const uint64_t*)s)[1] : (s[23] & 0x7f);
    const char* curStr = heap ? *(const char**)s : (const char*)s;

    std::string neededName;
    std::string correctName = lookupPlayerNameUniversal(actor, pid, num);
    if (!correctName.empty()) {
      if (!curStr || len == 0 || len != correctName.size() || strncmp(curStr, correctName.c_str(), len) != 0) {
        neededName = correctName;
      }
    } else if (curStr && len > 0) {
      std::string raw(curStr, len);
      std::string uc = zjn::sanitizeShirtName(raw.c_str());
      if (!uc.empty() && uc != raw) {
        neededName = uc;
      }
    }

    if (!neededName.empty()) {
      applyActorShirtName(actor, neededName);
    }

    *(uint8_t*)((uint8_t*)actor + 0xf10) = 1;
    *(uint8_t*)((uint8_t*)actor + 0x1aaf) = 1;
  }

  if (originalBackNumberStep) {
    originalBackNumberStep(actor);
  }

  if (actor) {
    *(uint8_t*)((uint8_t*)actor + 0xf10) = 1;
    *(uint8_t*)((uint8_t*)actor + 0x1aaf) = 1;

    void* nameTexActor = *(void**)((uint8_t*)actor + 0x1918);
    if (nameTexActor) {
      *(uint8_t*)((uint8_t*)nameTexActor + 0x290) = 0; // 0 = render player name; 1 = debug font test grid (orange box)

      const uint8_t* s = (const uint8_t*)actor + 0x1710;
      bool heap = (int8_t)s[23] < 0;
      uint64_t len = heap ? ((const uint64_t*)s)[1] : (s[23] & 0x7f);
      const char* curStr = heap ? *(const char**)s : (const char*)s;

      if (len > 0 && curStr && curStr[0]) {
        const uint8_t* ts = (const uint8_t*)nameTexActor + 0x278;
        bool theap = (int8_t)ts[23] < 0;
        uint64_t tlen = theap ? ((const uint64_t*)ts)[1] : (ts[23] & 0x7f);
        const char* tstr = theap ? *(const char**)ts : (const char*)ts;

        bool mismatch = (tlen == 0 || !tstr || tlen != len || strncmp(tstr, curStr, len) != 0);
        if (mismatch) {
          auto setNameFn = (void (*)(void*, void*))(0x100851098 + gSlide);
          if (setNameFn) setNameFn(nameTexActor, (uint8_t*)actor + 0x1710);
        }
      }
    }
  }
}

static void install() {
  intptr_t slide = 0; bool found = false;
  for (uint32_t i = 0; i < _dyld_image_count(); ++i) if (_dyld_get_image_header(i)->filetype == MH_EXECUTE) {
    slide = _dyld_get_image_vmaddr_slide(i); found = true; break;
  }
  if (!found) return;
  gSlide = slide;

  const uintptr_t slots[] = {0x105f177f8, 0x105f17800, 0x105f17808, 0x105f17818};
  const uintptr_t functions[] = {0x100851490, 0x100851894, 0x1008518c0, 0x1008511c8};
  for (int i = 0; i < 4; ++i) if (*(uintptr_t*)(slots[i] + slide) != functions[i] + slide) {
    ZLog(@"[JERSEY] unsupported native vtable; hooks disabled"); return;
  }
  vm_address_t page = (slots[0] + slide) & ~(vm_address_t)(vm_page_size - 1);
  vm_size_t length = slots[3] + slide + 8 - page;
  if (vm_protect(mach_task_self(), page, length, false, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY) != KERN_SUCCESS) {
    ZLog(@"[JERSEY] vtable protection failed; hooks disabled"); return;
  }
  originalLoad = (LoadFn)*(uintptr_t*)(slots[0] + slide);
  originalReady = (ReadyFn)*(uintptr_t*)(slots[1] + slide);
  originalDraw = (DrawFn)*(uintptr_t*)(slots[2] + slide);
  originalAsset = (AssetFn)*(uintptr_t*)(slots[3] + slide);
  *(uintptr_t*)(slots[0] + slide) = (uintptr_t)load;
  *(uintptr_t*)(slots[1] + slide) = (uintptr_t)ready;
  *(uintptr_t*)(slots[2] + slide) = (uintptr_t)draw;
  vm_protect(mach_task_self(), page, length, false, VM_PROT_READ);
  ZLog(@"[JERSEY] native name atlas fallback and diagnostics installed");

  const uintptr_t actorSlots[] = {
    0x105f286c0, 0x105f287a0, 0x105f28800,
    0x105f29098, 0x105f29178, 0x105f291d8
  };
  const uintptr_t actorFuncs[] = {
    0x10089f2e8, 0x1008a02f0, 0x10089f3ec,
    0x10089f2e8, 0x1008a02f0, 0x10089f3ec
  };

  // Hook Mobile vtable (slots 0, 1, 2)
  bool mobMatch = true;
  for (int i = 0; i < 3; ++i) {
    if (*(uintptr_t*)(actorSlots[i] + slide) != actorFuncs[i] + slide) {
      mobMatch = false; break;
    }
  }
  if (mobMatch) {
    vm_address_t actorPage = (actorSlots[0] + slide) & ~(vm_address_t)(vm_page_size - 1);
    vm_size_t actorLength = ((actorSlots[2] + slide + 8 - actorPage + vm_page_size - 1) & ~(vm_page_size - 1));
    if (vm_protect(mach_task_self(), actorPage, actorLength, false, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY) == KERN_SUCCESS) {
      originalSetUniformNumber = (SetUniformNumberFn)*(uintptr_t*)(actorSlots[0] + slide);
      originalSetupFromPlayerId = (SetupFromPlayerIdFn)*(uintptr_t*)(actorSlots[1] + slide);
      originalBackNumberStep = (BackNumberStepFn)*(uintptr_t*)(actorSlots[2] + slide);

      *(uintptr_t*)(actorSlots[0] + slide) = (uintptr_t)hookedSetUniformNumber;
      *(uintptr_t*)(actorSlots[1] + slide) = (uintptr_t)hookedSetupFromPlayerId;
      *(uintptr_t*)(actorSlots[2] + slide) = (uintptr_t)hookedBackNumberStep;
      vm_protect(mach_task_self(), actorPage, actorLength, false, VM_PROT_READ);
      ZLog(@"[JERSEY] player actor shirt name hooks installed for Mobile");
    } else {
      ZLog(@"[JERSEY] vm_protect failed for Mobile vtable");
    }
  } else {
    ZLog(@"[JERSEY] Mobile vtable mismatch; skipped");
  }

  // Hook MobileHigh vtable (slots 3, 4, 5)
  bool highMatch = true;
  for (int i = 3; i < 6; ++i) {
    if (*(uintptr_t*)(actorSlots[i] + slide) != actorFuncs[i] + slide) {
      highMatch = false; break;
    }
  }
  if (highMatch) {
    vm_address_t actorPage = (actorSlots[3] + slide) & ~(vm_address_t)(vm_page_size - 1);
    vm_size_t actorLength = ((actorSlots[5] + slide + 8 - actorPage + vm_page_size - 1) & ~(vm_page_size - 1));
    if (vm_protect(mach_task_self(), actorPage, actorLength, false, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY) == KERN_SUCCESS) {
      *(uintptr_t*)(actorSlots[3] + slide) = (uintptr_t)hookedSetUniformNumber;
      *(uintptr_t*)(actorSlots[4] + slide) = (uintptr_t)hookedSetupFromPlayerId;
      *(uintptr_t*)(actorSlots[5] + slide) = (uintptr_t)hookedBackNumberStep;
      vm_protect(mach_task_self(), actorPage, actorLength, false, VM_PROT_READ);
      ZLog(@"[JERSEY] player actor shirt name hooks installed for MobileHigh");
    } else {
      ZLog(@"[JERSEY] vm_protect failed for MobileHigh vtable");
    }
  } else {
    ZLog(@"[JERSEY] MobileHigh vtable mismatch; skipped");
  }
}
}
