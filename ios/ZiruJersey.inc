// Native name-atlas fallback, PESmobile 11.1.101 only. These data vtable slots
// are validated before writing; no executable page is modified at runtime.
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
  originalDraw(actor, canvas);
}

extern "C" int32_t gZiruHomeTeamId;
extern "C" int32_t gZiruAwayTeamId;

static std::string sanitizeShirtName(const char* s) {
  if (!s || !s[0]) return "";
  std::string out;
  out.reserve(24);
  for (size_t i = 0; s[i] != '\0' && out.size() < 24; ++i) {
    unsigned char c = (unsigned char)s[i];
    if (c >= 'a' && c <= 'z') {
      out += (char)(c - 'a' + 'A');
    } else if (c >= 'A' && c <= 'Z') {
      out += (char)c;
    } else if (c == ' ' || c == '.' || c == '-' || c == '\'') {
      out += (char)c;
    } else if (c == 0xC3 && s[i + 1] != '\0') {
      unsigned char c2 = (unsigned char)s[++i];
      if ((c2 >= 0x80 && c2 <= 0x85) || (c2 >= 0xA0 && c2 <= 0xA5)) out += 'A';
      else if (c2 == 0x87 || c2 == 0xA7) out += 'C';
      else if ((c2 >= 0x88 && c2 <= 0x8B) || (c2 >= 0xA8 && c2 <= 0xAB)) out += 'E';
      else if ((c2 >= 0x8C && c2 <= 0x8F) || (c2 >= 0xAC && c2 <= 0xAF)) out += 'I';
      else if (c2 == 0x91 || c2 == 0xB1) out += 'N';
      else if ((c2 >= 0x92 && c2 <= 0x96) || c2 == 0x98 || (c2 >= 0xB2 && c2 <= 0xB6) || c2 == 0xB8) out += 'O';
      else if ((c2 >= 0x99 && c2 <= 0x9C) || (c2 >= 0xB9 && c2 <= 0xBC)) out += 'U';
    }
  }
  while (!out.empty() && out.front() == ' ') out.erase(out.begin());
  while (!out.empty() && out.back() == ' ') out.pop_back();
  return (out.size() >= 1 && out.size() <= 24) ? out : "";
}

static std::string fallbackPlayerName(uint32_t baseId) {
  static const std::unordered_map<uint32_t, const char*> sMap = {
    {133580, "HAALAND"}, {38555, "MESSI"}, {7511, "C. RONALDO"}, {40352, "NEYMAR JR"},
    {110718, "MBAPPE"}, {44379, "DE BRUYNE"}, {138036, "BELLINGHAM"}, {117047, "VINI JR."},
    {109005, "RODRI"}, {45260, "SALAH"}, {40571, "LEWANDOWSKI"}, {47287, "KANE"},
    {45023, "SON"}, {32664, "MODRIC"}, {36770, "KROOS"}, {33702, "BENZEMA"},
    {36769, "SUAREZ"}, {42641, "GRIEZMANN"}, {131976, "SAKA"}, {118007, "FODEN"},
    {141673, "MUSIALA"}, {138384, "PEDRI"}, {145952, "GAVI"}, {167375, "LAMINE YAMAL"},
    {42668, "COURTOIS"}, {60512, "ALISSON"}, {103380, "EDERSON"}, {33145, "NEUER"},
    {45843, "VAN DIJK"}, {113351, "RUBEN"}, {126955, "SALIBA"}, {118073, "DAVIES"},
    {111451, "A. ARNOLD"}, {41103, "WALKER"}, {109265, "KIMMICH"}, {118485, "RICE"},
    {133465, "CAMAVINGA"}, {113408, "VALVERDE"}, {101057, "BERNARDO"}, {60587, "BRUNO F."},
    {110638, "RASHFORD"}, {118334, "LAUTARO"}, {117765, "OSIMHEN"}, {126280, "LEAO"},
    {140321, "KVARATSKHELIA"}, {109867, "BARELLA"}, {118047, "BASTONI"}, {113405, "T. HERNANDEZ"},
    {103445, "DIMARCO"}, {47000, "DYBALA"}, {44378, "MORATA"}, {120281, "ENZO"},
    {134907, "MAC ALLISTER"}, {127179, "JULIAN ALVAREZ"}, {108919, "R. DIAS"}, {100780, "CASEMIRO"},
    {100688, "MARQUINHOS"}, {46845, "GABRIEL JESUS"}, {45129, "DE GEA"}, {41571, "TER STEGEN"},
    {100236, "ODEGAARD"}, {114878, "GABRIEL"}, {4522, "INIESTA"}, {1082, "XAVI"},
    {2439, "PUYOL"}, {133543, "ZION SUZUKI"}, {134639, "CANNAVARO"}, {35496, "MERTESACKER"},
    {134594, "BONUCCI"}, {134632, "SUPHANAN"}, {137955, "JUN"}, {129961, "TOMIYASU"},
    {44840, "JORGINHO"}, {30210, "PARTEY"}, {137957, "TROSSARD"}, {110815, "HAVERTZ"},
    {107686, "MARTINELLI"}, {124266, "ZINCHENKO"}, {132933, "RAYA"}, {983, "BECKHAM"},
    {33185, "PIRLO"}, {134670, "MALDINI"}, {134701, "NEDVED"}, {34691, "NEDVED"},
    {45735, "MANE"}, {126343, "RODRYGO"}
  };
  auto it = sMap.find(baseId);
  return (it != sMap.end()) ? it->second : "";
}

static std::string lookupShirtName(void* actor, int uniformNumber) {
  if (uniformNumber <= 0 || uniformNumber > 99) return "";

  auto getMgr = (void* (*)())(0x104bba084 + gSlide);
  void* mgr = getMgr ? getMgr() : nullptr;
  if (!mgr) return "";

  uint8_t* base = *(uint8_t**)((uint8_t*)mgr + 0x48);
  if (!base) return "";

  uint8_t* match = base + 0x6c4c0;
  int32_t actorTeamId = actor ? *(int32_t*)((uint8_t*)actor + 0xe10) : -1;

  uint32_t homeTeamId = *(uint32_t*)(match + 0x11d0 + 0x10);
  uint32_t awayTeamId = *(uint32_t*)(match + 0x11d0 + 0x188 + 0x10);
  if (homeTeamId == 0 && gZiruHomeTeamId > 0) homeTeamId = (uint32_t)gZiruHomeTeamId;
  if (awayTeamId == 0 && gZiruAwayTeamId > 0) awayTeamId = (uint32_t)gZiruAwayTeamId;

  int preferredSide = -1;
  if (actorTeamId > 0) {
    if ((uint32_t)actorTeamId == homeTeamId) preferredSide = 0;
    else if ((uint32_t)actorTeamId == awayTeamId) preferredSide = 1;
  }

  auto checkSide = [&](int side) -> std::string {
    if (side < 0 || side > 1) return "";
    uint8_t* teamPlayers = match + 0x14e0 + (side * 0x5dc0);
    for (int i = 0; i < 40; ++i) {
      uint8_t* p = teamPlayers + (i * 0x258);
      uint16_t num = *(uint16_t*)(p + 0x1ec);
      if (num == (uint16_t)uniformNumber) {
        uint32_t baseId = *(uint32_t*)(p + 0x38);
        std::string name = sanitizeShirtName((const char*)(p + 0x89));
        if (name.empty()) name = sanitizeShirtName((const char*)(p + 0xc6));
        if (name.empty() && baseId > 0) name = fallbackPlayerName(baseId);
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

static std::string lookupShirtNameByPlayerId(int64_t playerId) {
  if (playerId <= 0) return "";
  uint32_t targetBaseId = (uint32_t)(playerId & 0xFFFFFF);

  auto getMgr = (void* (*)())(0x104bba084 + gSlide);
  void* mgr = getMgr ? getMgr() : nullptr;
  if (!mgr) return fallbackPlayerName(targetBaseId);

  uint8_t* base = *(uint8_t**)((uint8_t*)mgr + 0x48);
  if (!base) return fallbackPlayerName(targetBaseId);

  uint8_t* match = base + 0x6c4c0;

  for (int side = 0; side <= 1; ++side) {
    uint8_t* teamPlayers = match + 0x14e0 + (side * 0x5dc0);
    for (int i = 0; i < 40; ++i) {
      uint8_t* p = teamPlayers + (i * 0x258);
      uint32_t baseId = *(uint32_t*)(p + 0x38);
      if (baseId == targetBaseId) {
        std::string name = sanitizeShirtName((const char*)(p + 0x89));
        if (name.empty()) name = sanitizeShirtName((const char*)(p + 0xc6));
        if (!name.empty()) return name;
      }
    }
  }
  return fallbackPlayerName(targetBaseId);
}

static void applyActorShirtName(void* actor, const std::string& name) {
  if (!actor || name.empty()) return;
  auto setShirt = (void (*)(void*, const char*))(0x10090ef34 + gSlide);
  if (setShirt) {
    setShirt(actor, name.c_str());
  }
  *(uint8_t*)((uint8_t*)actor + 0xf10) = 1;

  void* nameTexActor = *(void**)((uint8_t*)actor + 0x1918);
  if (nameTexActor) {
    auto setNameFn = (void (*)(void*, void*))(0x100851098 + gSlide);
    if (setNameFn) setNameFn(nameTexActor, (uint8_t*)actor + 0x1710);
  }
}

static void hookedSetUniformNumber(void* actor, int uniformNumber) {
  if (originalSetUniformNumber) {
    originalSetUniformNumber(actor, uniformNumber);
  }
  if (!actor || uniformNumber <= 0) return;
  std::string name = lookupShirtName(actor, uniformNumber);
  if (name.empty()) {
    int64_t pid = *(int64_t*)((uint8_t*)actor + 0x1700);
    if (pid <= 0) pid = *(int32_t*)((uint8_t*)actor + 0x1708);
    name = lookupShirtNameByPlayerId(pid);
  }
  if (!name.empty()) {
    applyActorShirtName(actor, name);
    ZLog(@"[JERSEY] SetUniformNumber #%d (team %d) -> '%s'", uniformNumber,
        *(int32_t*)((uint8_t*)actor + 0xe10), name.c_str());
  }
}

static void hookedSetupFromPlayerId(void* actor, int64_t playerId, void* path, int uniformIndex, void* uniName) {
  if (originalSetupFromPlayerId) {
    originalSetupFromPlayerId(actor, playerId, path, uniformIndex, uniName);
  }
  if (!actor) return;
  int32_t num = *(int32_t*)((uint8_t*)actor + 0x1708);
  std::string name;
  if (num > 0) name = lookupShirtName(actor, num);
  if (name.empty() && playerId > 0) name = lookupShirtNameByPlayerId(playerId);
  if (!name.empty()) {
    applyActorShirtName(actor, name);
    ZLog(@"[JERSEY] SetupFromPlayerId pid=%lld num=%d -> '%s'", (long long)playerId, num, name.c_str());
  }
}

static void hookedBackNumberStep(void* actor) {
  if (actor) {
    int32_t num = *(int32_t*)((uint8_t*)actor + 0x1708);
    const uint8_t* s = (const uint8_t*)actor + 0x1710;
    bool heap = (int8_t)s[23] < 0;
    uint64_t len = heap ? ((const uint64_t*)s)[1] : (s[23] & 0x7f);
    if (len == 0 && num > 0) {
      std::string name = lookupShirtName(actor, num);
      if (name.empty()) {
        int64_t pid = *(int64_t*)((uint8_t*)actor + 0x1700);
        if (pid <= 0) pid = *(int32_t*)((uint8_t*)actor + 0x1708);
        name = lookupShirtNameByPlayerId(pid);
      }
      if (!name.empty()) {
        applyActorShirtName(actor, name);
      }
    } else if (len > 0) {
      *(uint8_t*)((uint8_t*)actor + 0xf10) = 1;
    }

    void* nameTexActor = *(void**)((uint8_t*)actor + 0x1918);
    if (nameTexActor && len > 0) {
      const uint8_t* ts = (const uint8_t*)nameTexActor + 0x278;
      bool theap = (int8_t)ts[23] < 0;
      uint64_t tlen = theap ? ((const uint64_t*)ts)[1] : (ts[23] & 0x7f);
      if (tlen == 0) {
        auto setNameFn = (void (*)(void*, void*))(0x100851098 + gSlide);
        if (setNameFn) setNameFn(nameTexActor, (uint8_t*)actor + 0x1710);
      }
    }
  }

  if (originalBackNumberStep) {
    originalBackNumberStep(actor);
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
