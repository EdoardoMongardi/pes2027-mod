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
static void asset(void* actor, NativeString* name, bool edit) { originalAsset(actor, name, edit); }
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


// Use the native actor's own name and number. Shirt numbers are not player IDs,
// and kit p1/p2 is not a home/away identifier. Never scan or mutate the roster
// from a render/tick callback.
static NSString* actorName(void* actor) {
  if (!actor) return @"<null>";
  const uint8_t* s = (const uint8_t*)actor + 0x1710;
  bool heap = (int8_t)s[23] < 0;
  uint64_t length = heap ? ((const uint64_t*)s)[1] : s[23];
  const char* text = heap ? *(const char**)s : (const char*)s;
  if (!text || length > 120) return @"<invalid>";
  return [[NSString alloc] initWithBytes:text length:(NSUInteger)length encoding:NSUTF8StringEncoding] ?: @"<invalid UTF-8>";
}
static std::atomic<int> numberLogBudget{96};
static void nativeSetUniformNumber(void* actor, int requested) {
  // Both Mobile vtables refer to this same native implementation; validated
  // at installation. Preserve its capture/LOD guards and the stored number.
  originalSetUniformNumber(actor, requested);
  if (!actor) return;
  int actual = *(int32_t*)((uint8_t*)actor + 0x1708);
  if (actual > 0 && actual <= 99) {
    *(uint8_t*)((uint8_t*)actor + 0xf10) = 1;
    *(uint8_t*)((uint8_t*)actor + 0x1aaf) = 1;
  }
  int remaining = numberLogBudget.load(std::memory_order_relaxed);
  while (remaining > 0 && !numberLogBudget.compare_exchange_weak(remaining, remaining - 1, std::memory_order_relaxed)) {}
  if (remaining > 0) {
    ZLog(@"[JERSEY_NATIVE] actor=%p requested=%d stored=%d name='%@' model=%d state=%d",
         actor, requested, actual, actorName(actor),
         *(int32_t*)((uint8_t*)actor + 0x10b8), *(int32_t*)((uint8_t*)actor + 0x1920));
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
  *(uintptr_t*)(slots[3] + slide) = (uintptr_t)asset;
  vm_protect(mach_task_self(), page, length, false, VM_PROT_READ);
  originalSetUniformNumber = (SetUniformNumberFn)(0x10089f2e8 + slide);
  for (uintptr_t slot : {0x105f286c0ULL, 0x105f29098ULL}) {
    if (*(uintptr_t*)(slot + slide) != (uintptr_t)originalSetUniformNumber) {
      ZLog(@"[JERSEY] native number vtable mismatch; leave unchanged"); continue;
    }
    vm_address_t p = (slot + slide) & ~(vm_address_t)(vm_page_size - 1);
    if (vm_protect(mach_task_self(), p, vm_page_size, false, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY) == KERN_SUCCESS) {
      *(uintptr_t*)(slot + slide) = (uintptr_t)nativeSetUniformNumber;
      vm_protect(mach_task_self(), p, vm_page_size, false, VM_PROT_READ);
    }
  }
  ZLog(@"[JERSEY_NATIVE] native names/numbers retained; roster guessing and per-frame name rewriting disabled");
}
}
