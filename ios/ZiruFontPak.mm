// See ZiruFontPak.h.
//
// Earlier versions mounted Ziru's font pak through UEBridgeSystem::MountPakFile; on device FPakFile::Initialize
// rejected it ("Index offset … is invalid") and aborted. This version mounts nothing: it replaces three slots of the
// FPakPlatformFile vtable (OpenRead, FileSize, OpenAsyncRead). When the engine asks for one of the four UI font files
// (…/Platforms/_IOS/Assets/ui/Font/<name>.ufont) the request is answered from the bundle copy
// (PESmobile.app/ziru/fonts/<name>.ufont) through the pak file's own lower-level platform file; every other path goes
// to the original functions untouched.
#include "ZiruFontPak.h"

#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach/mach.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atomic>
#include <string>

namespace {

// PESmobile 11.1.1 / 11.1.101 (sha256 0cf9bacd…a0de), unslid addresses.
const uint64_t kPakVtable = 0x10662b950;          // __ZTV16FPakPlatformFile
const uint64_t kSlotFileSize = kPakVtable + 0x10 + 0x80;
const uint64_t kSlotOpenRead = kPakVtable + 0x10 + 0xc8;
const uint64_t kSlotOpenAsyncRead = kPakVtable + 0x10 + 0x110;
const uint64_t kFnFileSize = 0x1029a605c;         // FPakPlatformFile::FileSize(const TCHAR*)
const uint64_t kFnOpenRead = 0x1029a4b30;         // FPakPlatformFile::OpenRead(const TCHAR*, bool)
const uint64_t kFnOpenAsyncRead = 0x102997684;    // FPakPlatformFile::OpenAsyncRead(const TCHAR*)
const size_t kLowerLevelOffset = 0x8;             // FPakPlatformFile::LowerLevel (see FPakPlatformFile::Mount)
// IPlatformFile vtable byte offsets (same layout for every platform file)
const size_t kVtFileSize = 0x80, kVtOpenRead = 0xc8, kVtOpenAsyncRead = 0x110;

const char* const kFonts[] = {"eFootballSans-Bold", "eFootballSans-Light", "eFootballSans-Regular",
                              "eFootballStencil-Regular"};
const int kNumFonts = 4;

typedef void* (*OpenReadFn)(void* self, const char16_t* name, bool allowWrite);
typedef int64_t (*FileSizeFn)(void* self, const char16_t* name);
typedef void* (*OpenAsyncReadFn)(void* self, const char16_t* name);
OpenReadFn gOrigOpenRead;
FileSizeFn gOrigFileSize;
OpenAsyncReadFn gOrigOpenAsyncRead;

std::u16string gFontPath[kNumFonts];   // bundle paths as TCHAR
std::atomic<int> gHits[kNumFonts];
void (*gLog)(const char*) = nullptr;

void logf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void logf(const char* fmt, ...) {
  if (!gLog) return;
  char buf[1024];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);
  gLog(buf);
}

std::u16string widen(const std::string& s) { return std::u16string(s.begin(), s.end()); }   // bundle paths are ASCII

// Index of the font file `name` refers to, or -1. Matches ".../Assets/ui/Font/<font>.ufont" (case-insensitive).
int fontIndex(const char16_t* name) {
  if (!name) return -1;
  size_t n = 0;
  while (name[n]) n++;
  for (int i = 0; i < kNumFonts; i++) {
    std::string want = std::string("/assets/ui/font/") + kFonts[i] + ".ufont";
    size_t w = want.size();
    if (n < w) continue;
    bool ok = true;
    for (size_t k = 0; k < w && ok; k++) {
      char16_t c = name[n - w + k];
      if (c == u'\\') c = u'/';
      if (c >= u'A' && c <= u'Z') c = (char16_t)(c - u'A' + u'a');
      char d = want[k];
      if (d >= 'A' && d <= 'Z') d = (char)(d - 'A' + 'a');
      ok = c == (char16_t)d;
    }
    if (ok) return i;
  }
  return -1;
}

inline void* lowerOf(void* self) { return *(void**)((char*)self + kLowerLevelOffset); }
template <typename F>
inline F vfn(void* obj, size_t off) {
  return *(F*)(*(char**)obj + off);
}

void noteHit(int i, const char* op, bool ok) {
  if (gHits[i].fetch_add(1) < 3) logf("[FONT] %s %s.ufont -> Ziru font from bundle: %s", op, kFonts[i], ok ? "ok" : "FAILED (using original)");
}

void* hookOpenRead(void* self, const char16_t* name, bool allowWrite) {
  int i = fontIndex(name);
  if (i >= 0 && !allowWrite) {
    void* lower = lowerOf(self);
    void* h = lower ? vfn<OpenReadFn>(lower, kVtOpenRead)(lower, gFontPath[i].c_str(), false) : nullptr;
    noteHit(i, "OpenRead", h != nullptr);
    if (h) return h;
  }
  return gOrigOpenRead(self, name, allowWrite);
}

int64_t hookFileSize(void* self, const char16_t* name) {
  int i = fontIndex(name);
  if (i >= 0) {
    void* lower = lowerOf(self);
    int64_t s = lower ? vfn<FileSizeFn>(lower, kVtFileSize)(lower, gFontPath[i].c_str()) : -1;
    if (s > 0) return s;
  }
  return gOrigFileSize(self, name);
}

void* hookOpenAsyncRead(void* self, const char16_t* name) {
  int i = fontIndex(name);
  if (i >= 0) {
    void* lower = lowerOf(self);
    void* h = lower ? vfn<OpenAsyncReadFn>(lower, kVtOpenAsyncRead)(lower, gFontPath[i].c_str()) : nullptr;
    noteHit(i, "OpenAsyncRead", h != nullptr);
    if (h) return h;
  }
  return gOrigOpenAsyncRead(self, name);
}

void patchSlot(intptr_t slide, uint64_t slotVA, void* repl, void** orig) {
  void** slot = (void**)(uintptr_t)(slotVA + slide);
  *orig = *slot;
  *slot = repl;
}

}  // namespace

namespace zfont {

void install(const char* fontDir, const char* documentsDir, void (*log)(const char* line)) {
  gLog = log;
  // Leftovers of the previous (pak-mount) attempt: our own small files, never game data.
  if (documentsDir) {
    std::string old = std::string(documentsDir) + "/pak/ZiruFont_IOS_P.pak";
    std::string guard = std::string(documentsDir) + "/PES2027-logs/font-mount.pending";
    if (unlink(old.c_str()) == 0) logf("[FONT] removed old %s", old.c_str());
    unlink(guard.c_str());
  }
  if (!fontDir) return;
  for (int i = 0; i < kNumFonts; i++) {
    std::string p = std::string(fontDir) + "/" + kFonts[i] + ".ufont";
    struct stat st;
    if (stat(p.c_str(), &st) != 0 || st.st_size < 1024) {
      logf("[FONT] %s missing in the app bundle – Vietnamese font fix disabled", p.c_str());
      return;
    }
    gFontPath[i] = widen(p);
  }
  intptr_t slide = 0;
  bool found = false;
  for (uint32_t i = 0; i < _dyld_image_count(); i++) {
    const struct mach_header* h = _dyld_get_image_header(i);
    if (h && h->filetype == MH_EXECUTE) {
      slide = _dyld_get_image_vmaddr_slide(i);
      found = true;
      break;
    }
  }
  if (!found) {
    logf("[FONT] main executable not found");
    return;
  }
  // All three slots must match the analysed binary before anything is changed (all-or-nothing).
  const uint64_t slots[3] = {kSlotOpenRead, kSlotFileSize, kSlotOpenAsyncRead};
  const uint64_t fns[3] = {kFnOpenRead, kFnFileSize, kFnOpenAsyncRead};
  for (int k = 0; k < 3; k++) {
    void* cur = *(void**)(uintptr_t)(slots[k] + slide);
    if (cur != (void*)(uintptr_t)(fns[k] + slide)) {
      logf("[FONT] FPakPlatformFile vtable slot %d holds %p, expected %p – font fix disabled", k, cur,
           (void*)(uintptr_t)(fns[k] + slide));
      return;
    }
  }
  // The three slots are within 0x90 bytes; protect the page range covering them.
  vm_address_t lo = (vm_address_t)(kSlotFileSize + slide) & ~(vm_address_t)(vm_page_size - 1);
  vm_address_t hi = (vm_address_t)(kSlotOpenAsyncRead + slide + 8);
  vm_size_t len = hi - lo;
  if (vm_protect(mach_task_self(), lo, len, false, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY) != KERN_SUCCESS) {
    logf("[FONT] vm_protect failed – font fix disabled");
    return;
  }
  patchSlot(slide, kSlotOpenRead, (void*)hookOpenRead, (void**)&gOrigOpenRead);
  patchSlot(slide, kSlotFileSize, (void*)hookFileSize, (void**)&gOrigFileSize);
  patchSlot(slide, kSlotOpenAsyncRead, (void*)hookOpenAsyncRead, (void**)&gOrigOpenAsyncRead);
  vm_protect(mach_task_self(), lo, len, false, VM_PROT_READ);
  logf("[FONT] installed: FPakPlatformFile OpenRead/FileSize/OpenAsyncRead serve %d UI fonts from %s", kNumFonts, fontDir);
}

}  // namespace zfont
