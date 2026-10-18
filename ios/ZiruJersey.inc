// Native name-atlas fallback, PESmobile 11.1.101 only. These data vtable slots
// are validated before writing; no executable page is modified at runtime.
namespace zjersey {
using LoadFn = void (*)(void*);
using ReadyFn = bool (*)(void*);
using DrawFn = void (*)(void*, void*);
struct NativeString { char16_t* data; int32_t num; int32_t max; };
using AssetFn = void (*)(void*, NativeString*, bool);
using SetupFromPlayerIdFn = void (*)(void* actor, int64_t playerId, void* path, int uniformIndex, void* uniName);
using SetupBackNumberAndNameFn = void (*)(void* actor);

static LoadFn originalLoad;
static ReadyFn originalReady;
static DrawFn originalDraw;
static AssetFn originalAsset;
static SetupFromPlayerIdFn originalSetupFromPlayerId;
static SetupBackNumberAndNameFn originalSetupBackNumberAndName;

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

static void setActorShirtName(void* actor, int64_t playerId, intptr_t slide) {
  if (!actor || playerId <= 0) return;

  const char* pName = nullptr;
  auto getMgr = (void* (*)())(0x104bba084 + slide);
  auto findRec = (void* (*)(void*, int, int64_t))(0x104b9588c + slide);
  auto getName = (const char* (*)(void*))(0x104bc2460 + slide);
  auto setShirt = (void (*)(void*, const char*))(0x10090ef34 + slide);

  void* mgr = getMgr ? getMgr() : nullptr;
  if (mgr) {
    void* db = *(void**)((uint8_t*)mgr + 0x40);
    if (db && findRec && getName) {
      void* rec = findRec(db, 0, playerId);
      if (rec) pName = getName(rec);
    }
  }

  if (pName && pName[0] && setShirt) {
    std::string upper;
    for (const char* p = pName; *p; ++p) upper += (char)toupper((unsigned char)*p);
    setShirt(actor, upper.c_str());
    *(uint8_t*)((uint8_t*)actor + 0xf10) = 1; // enable uniform name drawing flag
    ZLog(@"[JERSEY] Set shirt name for player %lld -> '%s'", (long long)playerId, upper.c_str());

    void* nameTexActor = *(void**)((uint8_t*)actor + 0x1918);
    if (nameTexActor) {
      auto setNameFn = (void (*)(void*, void*))(0x100851098 + slide);
      if (setNameFn) setNameFn(nameTexActor, (uint8_t*)actor + 0x1710);
    }
  }
}

static void hookedSetupFromPlayerId(void* actor, int64_t playerId, void* path, int uniformIndex, void* uniName) {
  if (originalSetupFromPlayerId) {
    originalSetupFromPlayerId(actor, playerId, path, uniformIndex, uniName);
  }
  setActorShirtName(actor, playerId, gSlide);
}

static void hookedSetupBackNumberAndName(void* actor) {
  if (originalSetupBackNumberAndName) {
    originalSetupBackNumberAndName(actor);
  }
  if (!actor) return;
  const uint8_t* s = (const uint8_t*)actor + 0x1710;
  bool heap = (int8_t)s[23] < 0;
  uint64_t length = heap ? ((const uint64_t*)s)[1] : s[23];
  if (length > 0) {
    void* nameTexActor = *(void**)((uint8_t*)actor + 0x1918);
    if (nameTexActor) {
      auto setNameFn = (void (*)(void*, void*))(0x100851098 + gSlide);
      if (setNameFn) setNameFn(nameTexActor, (uint8_t*)actor + 0x1710);
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

  const uintptr_t actorSlots[] = {0x106f2dd08, 0x106f2dd18};
  const uintptr_t actorFuncs[] = {0x10089f3ec, 0x1008a02f0};
  bool actorVtableMatch = true;
  for (int i = 0; i < 2; ++i) {
    if (*(uintptr_t*)(actorSlots[i] + slide) != actorFuncs[i] + slide) {
      actorVtableMatch = false; break;
    }
  }
  if (actorVtableMatch) {
    vm_address_t actorPage = (actorSlots[0] + slide) & ~(vm_address_t)(vm_page_size - 1);
    vm_size_t actorLength = ((actorSlots[1] + slide + 8 - actorPage + vm_page_size - 1) & ~(vm_page_size - 1));
    if (vm_protect(mach_task_self(), actorPage, actorLength, false, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY) == KERN_SUCCESS) {
      originalSetupBackNumberAndName = (SetupBackNumberAndNameFn)*(uintptr_t*)(actorSlots[0] + slide);
      originalSetupFromPlayerId = (SetupFromPlayerIdFn)*(uintptr_t*)(actorSlots[1] + slide);
      *(uintptr_t*)(actorSlots[0] + slide) = (uintptr_t)hookedSetupBackNumberAndName;
      *(uintptr_t*)(actorSlots[1] + slide) = (uintptr_t)hookedSetupFromPlayerId;
      vm_protect(mach_task_self(), actorPage, actorLength, false, VM_PROT_READ);
      ZLog(@"[JERSEY] player actor shirt name hooks installed successfully");
    } else {
      ZLog(@"[JERSEY] vm_protect failed for player actor vtable");
    }
  } else {
    ZLog(@"[JERSEY] player actor vtable mismatch; name injection skipped");
  }
}
}
