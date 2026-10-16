// See ZiruFontPak.h.
#include "ZiruFontPak.h"

#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach/mach.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <atomic>
#include <string>

namespace {

// Addresses in PESmobile 11.1.1 / 11.1.101 (sha256 0cf9bacd…a0de), unslid.
const uint64_t kVtSlotMountPak = 0x105eda568;   // __ZTV14UEBridgeSystem + 0x260 → UEBridgeSystem::MountPakFile
const uint64_t kBridgeMountPak = 0x10069caa4;   // UEBridgeSystem::MountPakFile(const char*, uint32, const char*, int64)
const uint32_t kBridgeMountCode[5] = {
    0xAA0103E0,   // mov x0, x1
    0xAA0203E1,   // mov x1, x2
    0xAA0303E2,   // mov x2, x3
    0xAA0403E3,   // mov x3, x4
    0x142D2B57,   // b UESystem::MountPakFile (0x1011e7810)
};
const uint32_t kFontPakOrder = 1000000;   // above every game pak → our .ufont files win

typedef bool (*MountFn)(void* self, const char* path, uint32_t order, const char* mountPoint, int64_t extra);
MountFn gOrigMount = nullptr;
std::string gPakPath;
std::atomic<bool> gFontMounted{false};
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

bool hookMount(void* self, const char* path, uint32_t order, const char* mountPoint, int64_t extra) {
  bool ok = gOrigMount(self, path, order, mountPoint, extra);
  const char* name = path ? strrchr(path, '/') : nullptr;
  logf("[PAK] MountPakFile %s order=%u mount=%s -> %s", name ? name + 1 : (path ? path : "(null)"), order,
       mountPoint ? mountPoint : "(pak)", ok ? "ok" : "FAILED");
  if (ok && !gFontMounted.exchange(true)) {
    bool f = gOrigMount(self, gPakPath.c_str(), kFontPakOrder, nullptr, 0);
    logf("[FONT] Vietnamese font pak %s (order %u) -> %s", gPakPath.c_str(), kFontPakOrder, f ? "MOUNTED" : "FAILED");
    if (!f) gFontMounted = false;   // try again after the next game mount
  }
  return ok;
}

}  // namespace

namespace zfont {

void install(const char* pakPath, void (*log)(const char* line)) {
  gLog = log;
  struct stat st;
  if (!pakPath || stat(pakPath, &st) != 0) {
    logf("[FONT] %s not in the app bundle – Vietnamese font fix disabled", pakPath ? pakPath : "(null)");
    return;
  }
  gPakPath = pakPath;
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
  // Only patch the binary we analysed: check the bridge thunk's code and the vtable slot's current value.
  const uint32_t* code = (const uint32_t*)(uintptr_t)(kBridgeMountPak + slide);
  if (memcmp(code, kBridgeMountCode, sizeof kBridgeMountCode) != 0) {
    logf("[FONT] PESmobile layout differs (MountPakFile thunk) – font fix disabled");
    return;
  }
  void** slot = (void**)(uintptr_t)(kVtSlotMountPak + slide);
  if (*slot != (void*)(uintptr_t)(kBridgeMountPak + slide)) {
    logf("[FONT] vtable slot holds %p, expected %p – font fix disabled", *slot, (void*)(uintptr_t)(kBridgeMountPak + slide));
    return;
  }
  vm_address_t page = (vm_address_t)slot & ~(vm_address_t)(vm_page_size - 1);
  kern_return_t kr = vm_protect(mach_task_self(), page, vm_page_size, false, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY);
  if (kr != KERN_SUCCESS) {
    logf("[FONT] vm_protect failed (%d) – font fix disabled", kr);
    return;
  }
  gOrigMount = (MountFn)*slot;
  *slot = (void*)hookMount;
  vm_protect(mach_task_self(), page, vm_page_size, false, VM_PROT_READ);
  logf("[FONT] MountPakFile hooked; will mount %s (%lld bytes) after the first game pak", pakPath, (long long)st.st_size);
}

}  // namespace zfont
