// ZiruOffline.dylib — loaded into eFootball iOS 11.1.1 (PESmobile).
//  * Runs the C++ port of the Ziru offline server in-process.
//  * Redirects the game's HttpImpl* requests to https://127.0.0.1:8085 (same as Ziru's Android HttpImpl patch)
//    and answers them through an NSURLProtocol — no sockets, no TLS.
//  * Gates the UE4 launch until the separate data pack (pak/cpk/livedata) has been imported.
//  * Diagnostics: fileio.log, crash.log, ziru.log, offline_server.log, network logging.
//  * Commentary protection: prevents deletion of dt510 Vietnamese commentary.
//  * PES21 stadium design with unremovable 'Made by iOSVN' watermark.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#import <mach-o/dyld.h>
#import <mach-o/loader.h>
#import <mach-o/nlist.h>
#import <dlfcn.h>
#import <signal.h>
#import <sys/stat.h>
#import <fcntl.h>
#import <unistd.h>
#import <mach/mach.h>
#import <mach/vm_map.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>

static thread_local bool gInHook = false;
static int (*orig_open)(const char *path, int oflag, ...);
static int (*orig_access)(const char *path, int amode);
static int (*orig_stat)(const char *path, struct stat *buf);
static int (*orig_lstat)(const char *path, struct stat *buf);
static int (*orig_unlink)(const char *path);
static int (*orig_unlinkat)(int fd, const char *path, int flag);
static int (*orig_remove)(const char *path);

#include "../core/datapack.hpp"
#include "../core/server.hpp"

#define ZIRU_MOD_VERSION "1.0.1"

// ---------------------------------------------------------------- theme colors (PES21)
static inline UIColor* PESLime() { return [UIColor colorWithRed:0.77 green:0.98 blue:0.22 alpha:1.0]; }
static inline UIColor* PESNight() { return [UIColor colorWithRed:0.025 green:0.045 blue:0.065 alpha:1.0]; }
static inline UIColor* PESCyan() { return [UIColor colorWithRed:0.27 green:0.86 blue:0.92 alpha:1.0]; }
static inline UIColor* PESMuted() { return [UIColor colorWithRed:0.65 green:0.73 blue:0.75 alpha:1.0]; }

// ---------------------------------------------------------------- paths & logs
static NSString* gDocs;
static NSString* gSupport;   // Library/Application Support/PES2027
static NSString* gLogPath;
static NSString* gFileIoLogPath;
static NSString* gCrashLogPath;
static NSString* gBundleAssets;
static std::mutex gLogMu;

static void ZLog(NSString* fmt, ...) NS_FORMAT_FUNCTION(1, 2);
static void ZLog(NSString* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  NSString* s = [[NSString alloc] initWithFormat:fmt arguments:ap];
  va_end(ap);
  NSLog(@"[Ziru] %@", s);
  if (!gLogPath) return;
  std::lock_guard<std::mutex> g(gLogMu);
  static NSDateFormatter* df;
  if (!df) { df = [NSDateFormatter new]; df.dateFormat = @"HH:mm:ss.SSS"; }
  NSString* line = [NSString stringWithFormat:@"%@ %@\n", [df stringFromDate:[NSDate date]], s];
  FILE* f = fopen(gLogPath.fileSystemRepresentation, "a");
  if (f) { fputs(line.UTF8String, f); fclose(f); }
}

static void logSink(int level, const char* tag, const char* msg) {
  if (level >= 2) ZLog(@"%s %s", tag, msg);
}

static zd::Layout layout() {
  zd::Layout L;
  L.gameDataRoot = std::string(gSupport.fileSystemRepresentation) + "/GameData";
  L.documents = gDocs.fileSystemRepresentation;
  return L;
}

static void initPaths() {
  gDocs = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
  NSString* lib = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES).firstObject;
  gSupport = [lib stringByAppendingPathComponent:@"PES2027"];
  NSFileManager* fm = NSFileManager.defaultManager;
  [fm createDirectoryAtPath:[gSupport stringByAppendingPathComponent:@"GameData"] withIntermediateDirectories:YES attributes:nil error:nil];
  [fm createDirectoryAtPath:[gSupport stringByAppendingPathComponent:@"Save"] withIntermediateDirectories:YES attributes:nil error:nil];
  // Data and saves are not iCloud-backed-up (several GB)
  NSURL* u = [NSURL fileURLWithPath:gSupport];
  [u setResourceValue:@YES forKey:NSURLIsExcludedFromBackupKey error:nil];
  NSString* logDir = [gDocs stringByAppendingPathComponent:@"PES2027-logs"];
  [fm createDirectoryAtPath:logDir withIntermediateDirectories:YES attributes:nil error:nil];
  gLogPath = [logDir stringByAppendingPathComponent:@"ziru.log"];
  gFileIoLogPath = [logDir stringByAppendingPathComponent:@"fileio.log"];
  gCrashLogPath = [logDir stringByAppendingPathComponent:@"crash.log"];

  // Log rotation (> 8MB)
  for (NSString* p in @[gLogPath, gFileIoLogPath]) {
    NSDictionary* a = [fm attributesOfItemAtPath:p error:nil];
    if (a && [a fileSize] > (8u << 20)) {
      NSString* old = [p stringByAppendingString:@".old"];
      [fm removeItemAtPath:old error:nil];
      [fm moveItemAtPath:p toPath:old error:nil];
    }
  }

  gBundleAssets = [NSBundle.mainBundle.bundlePath stringByAppendingPathComponent:@"ziru"];
  zs::setLogSink(logSink);
}

// ---------------------------------------------------------------- crash handler (crash.log)
static void crashHandler(int sig, siginfo_t* info, void* ucontext) {
  static volatile sig_atomic_t crashed = 0;
  if (crashed++) _exit(1);
  gInHook = true;

  uintptr_t pc = 0, lr = 0, fp = 0, faultAddr = (uintptr_t)(info ? info->si_addr : 0);
#if defined(__arm64__)
  ucontext_t* uc = (ucontext_t*)ucontext;
  if (uc && uc->uc_mcontext) {
    pc = uc->uc_mcontext->__ss.__pc;
    lr = uc->uc_mcontext->__ss.__lr;
    fp = uc->uc_mcontext->__ss.__fp;
  }
#endif

  intptr_t mainSlide = _dyld_get_image_vmaddr_slide(0);
  const char* mainName = _dyld_get_image_name(0);
  const char* baseName = mainName ? (strrchr(mainName, '/') ? strrchr(mainName, '/') + 1 : mainName) : "PESmobile";
  uintptr_t relPC = (pc >= (uintptr_t)mainSlide) ? (pc - (uintptr_t)mainSlide) : 0;
  uintptr_t relLR = (lr >= (uintptr_t)mainSlide) ? (lr - (uintptr_t)mainSlide) : 0;

  int fd = -1;
  if (gCrashLogPath) {
    fd = orig_open ? orig_open(gCrashLogPath.fileSystemRepresentation, O_WRONLY | O_CREAT | O_APPEND, 0644)
                   : open(gCrashLogPath.fileSystemRepresentation, O_WRONLY | O_CREAT | O_APPEND, 0644);
  }
  if (fd < 0) fd = 2;

  time_t now = time(nullptr);
  char timeBuf[64];
  strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", localtime(&now));

  dprintf(fd, "\n==================== PES2027 CRASH REPORT ====================\n");
  dprintf(fd, "Timestamp: %s\n", timeBuf);
  dprintf(fd, "Signal: %d (%s)\n", sig, strsignal(sig));
  dprintf(fd, "Fault Address: 0x%lx\n", (unsigned long)faultAddr);
  dprintf(fd, "PC: 0x%lx (relative to %s: 0x%lx)\n", (unsigned long)pc, baseName, (unsigned long)relPC);
  dprintf(fd, "LR: 0x%lx (relative: 0x%lx)\n", (unsigned long)lr, (unsigned long)relLR);
  dprintf(fd, "FP: 0x%lx\n", (unsigned long)fp);
  dprintf(fd, "ASLR Slide: 0x%lx\n", (unsigned long)mainSlide);
  dprintf(fd, "\n--- Backtrace ---\n");

  @try {
    NSArray<NSString*>* syms = [NSThread callStackSymbols];
    if (syms) {
      for (NSUInteger i = 0; i < syms.count; ++i) {
        dprintf(fd, "#%02lu %s\n", (unsigned long)i, syms[i].UTF8String);
      }
    }
  } @catch (...) {}
  dprintf(fd, "==============================================================\n\n");
  if (fd > 2) close(fd);
  _exit(128 + sig);
}

static void initCrashHandler() {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = crashHandler;
  sa.sa_flags = SA_SIGINFO | SA_NODEFER | SA_RESETHAND;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGBUS, &sa, NULL);
  sigaction(SIGABRT, &sa, NULL);
  sigaction(SIGILL, &sa, NULL);
  sigaction(SIGFPE, &sa, NULL);
  sigaction(SIGTRAP, &sa, NULL);
}

// ---------------------------------------------------------------- fishhook (Mach-O symbol rebinding)
struct z_rebinding {
  const char *name;
  void *replacement;
  void **replaced;
};

struct z_rebindings_entry {
  struct z_rebinding *rebindings;
  size_t rebindings_nel;
  struct z_rebindings_entry *next;
};

static struct z_rebindings_entry *_z_rebindings_head = NULL;

static int z_prepend_rebindings(struct z_rebindings_entry **rebindings_head,
                                struct z_rebinding rebindings[],
                                size_t nel) {
  struct z_rebindings_entry *new_entry = (struct z_rebindings_entry *)malloc(sizeof(struct z_rebindings_entry));
  if (!new_entry) return -1;
  new_entry->rebindings = (struct z_rebinding *)malloc(sizeof(struct z_rebinding) * nel);
  if (!new_entry->rebindings) {
    free(new_entry);
    return -1;
  }
  memcpy(new_entry->rebindings, rebindings, sizeof(struct z_rebinding) * nel);
  new_entry->rebindings_nel = nel;
  new_entry->next = *rebindings_head;
  *rebindings_head = new_entry;
  return 0;
}

static void z_perform_rebinding_with_section(struct z_rebindings_entry *rebindings,
                                             section_64 *sect,
                                             intptr_t slide,
                                             nlist_64 *symtab,
                                             char *strtab,
                                             uint32_t *indirect_symtab) {
  uint32_t *indirect_symbol_indices = indirect_symtab + sect->reserved1;
  void **indirect_symbol_bindings = (void **)((uintptr_t)slide + sect->addr);

  // vm_protect to allow writing to read-only sections like __DATA_CONST
  vm_address_t page_addr = (vm_address_t)indirect_symbol_bindings;
  vm_size_t page_size = (vm_size_t)sect->size;
  kern_return_t kr = vm_protect(mach_task_self(), page_addr, page_size, 0, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY);
  if (kr != KERN_SUCCESS) return;

  for (uint32_t i = 0; i < sect->size / sizeof(void *); i++) {
    uint32_t symtab_index = indirect_symbol_indices[i];
    if (symtab_index == INDIRECT_SYMBOL_ABS || symtab_index == INDIRECT_SYMBOL_LOCAL ||
        symtab_index == (INDIRECT_SYMBOL_LOCAL | INDIRECT_SYMBOL_ABS)) {
      continue;
    }
    uint32_t strtab_offset = symtab[symtab_index].n_un.n_strx;
    char *symbol_name = strtab + strtab_offset;
    bool has_leading_underscore = symbol_name[0] == '_';
    struct z_rebindings_entry *cur = rebindings;
    while (cur) {
      for (size_t j = 0; j < cur->rebindings_nel; j++) {
        const char *want = cur->rebindings[j].name;
        if (has_leading_underscore && strcmp(&symbol_name[1], want) == 0) {
          if (cur->rebindings[j].replaced != NULL &&
              *(cur->rebindings[j].replaced) == NULL &&
              indirect_symbol_bindings[i] != cur->rebindings[j].replacement) {
            *(cur->rebindings[j].replaced) = indirect_symbol_bindings[i];
          }
          indirect_symbol_bindings[i] = cur->rebindings[j].replacement;
          goto symbol_done;
        }
      }
      cur = cur->next;
    }
  symbol_done:;
  }
}

static void z_rebind_symbols_for_image(struct z_rebindings_entry *rebindings,
                                       const struct mach_header *header,
                                       intptr_t slide) {
  Dl_info info;
  if (dladdr(header, &info) == 0) return;

  segment_command_64 *cur_seg_cmd;
  segment_command_64 *linkedit_segment = NULL;
  struct symtab_command *symtab_cmd = NULL;
  struct dysymtab_command *dysymtab_cmd = NULL;

  uintptr_t cur = (uintptr_t)header + sizeof(mach_header_64);
  for (uint32_t i = 0; i < header->ncmds; i++, cur += cur_seg_cmd->cmdsize) {
    cur_seg_cmd = (segment_command_64 *)cur;
    if (cur_seg_cmd->cmd == LC_SEGMENT_64) {
      if (strcmp(cur_seg_cmd->segname, SEG_LINKEDIT) == 0) {
        linkedit_segment = cur_seg_cmd;
      }
    } else if (cur_seg_cmd->cmd == LC_SYMTAB) {
      symtab_cmd = (struct symtab_command *)cur_seg_cmd;
    } else if (cur_seg_cmd->cmd == LC_DYSYMTAB) {
      dysymtab_cmd = (struct dysymtab_command *)cur_seg_cmd;
    }
  }

  if (!linkedit_segment || !symtab_cmd || !dysymtab_cmd) return;

  uintptr_t linkedit_base = (uintptr_t)slide + linkedit_segment->vmaddr - linkedit_segment->fileoff;
  nlist_64 *symtab = (nlist_64 *)(linkedit_base + symtab_cmd->symoff);
  char *strtab = (char *)(linkedit_base + symtab_cmd->stroff);
  uint32_t *indirect_symtab = (uint32_t *)(linkedit_base + dysymtab_cmd->indirectsymoff);

  cur = (uintptr_t)header + sizeof(mach_header_64);
  for (uint32_t i = 0; i < header->ncmds; i++, cur += cur_seg_cmd->cmdsize) {
    cur_seg_cmd = (segment_command_64 *)cur;
    if (cur_seg_cmd->cmd == LC_SEGMENT_64) {
      if (strcmp(cur_seg_cmd->segname, SEG_DATA) != 0 &&
          strcmp(cur_seg_cmd->segname, "__DATA_CONST") != 0) {
        continue;
      }
      for (uint32_t j = 0; j < cur_seg_cmd->nsects; j++) {
        section_64 *sect = (section_64 *)(cur + sizeof(segment_command_64)) + j;
        uint32_t type = sect->flags & SECTION_TYPE;
        if (type == S_LAZY_SYMBOL_POINTERS || type == S_NON_LAZY_SYMBOL_POINTERS) {
          z_perform_rebinding_with_section(rebindings, sect, slide, symtab, strtab, indirect_symtab);
        }
      }
    }
  }
}

static int z_rebind_symbols(struct z_rebinding rebindings[], size_t nel) {
  int r = z_prepend_rebindings(&_z_rebindings_head, rebindings, nel);
  if (r < 0) return r;
  // Rebind ONLY for the main executable PESmobile (image 0)
  // NEVER hook dyld shared cache or system frameworks, which causes crash on iOS
  const struct mach_header *header = _dyld_get_image_header(0);
  intptr_t slide = _dyld_get_image_vmaddr_slide(0);
  if (header) {
    z_rebind_symbols_for_image(_z_rebindings_head, header, slide);
  }
  return 0;
}

// ---------------------------------------------------------------- file I/O interception & BLV protection
static BOOL isTrackedPath(const char* path) {
  if (!path) return NO;
  if (strstr(path, "PES2027-logs") || strstr(path, ".log") || strstr(path, "ZiruOffline")) return NO;
  if (strstr(path, "cpk") || strstr(path, "pak") || strstr(path, "Download") ||
      strstr(path, "dt510") || strstr(path, "dt500") || strstr(path, "dt2") ||
      strstr(path, "GameData") || strstr(path, "livedata") || strstr(path, "version") ||
      strstr(path, "manifest") || strstr(path, "sound") || strstr(path, ".acb") ||
      strstr(path, ".awb")) {
    return YES;
  }
  return NO;
}

static BOOL isCommentaryPath(const char* path) {
  if (!path) return NO;
  return (strstr(path, "dt510") != nullptr || strstr(path, "dt510_eng_mobile_all.cpk") != nullptr);
}

static void logFileIo(const char* op, const char* path, int res, int err) {
  if (!gFileIoLogPath || !isTrackedPath(path)) return;
  if (gInHook) return;
  gInHook = true;
  {
    std::lock_guard<std::mutex> g(gLogMu);
    int fd = orig_open ? orig_open(gFileIoLogPath.fileSystemRepresentation, O_WRONLY | O_CREAT | O_APPEND, 0644)
                       : open(gFileIoLogPath.fileSystemRepresentation, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
      time_t now = time(nullptr);
      char tbuf[32];
      strftime(tbuf, sizeof(tbuf), "%H:%M:%S", localtime(&now));
      char line[1024];
      int len = 0;
      if (res == 0) {
        len = snprintf(line, sizeof(line), "%s [%s] OK: %s\n", tbuf, op, path);
      } else {
        len = snprintf(line, sizeof(line), "%s [%s] FAIL(%d: %s): %s\n", tbuf, op, err, strerror(err), path);
      }
      if (len > 0) {
        write(fd, line, (size_t)len);
      }
      close(fd);
    }
  }
  gInHook = false;
}

static int hook_unlink(const char *path) {
  if (isCommentaryPath(path)) {
    ZLog(@"[BLV_PROTECT] Prevented deletion (unlink) of commentary file: %s", path);
    logFileIo("UNLINK_PROTECTED", path, 0, 0);
    return 0; // Fake success
  }
  if (gInHook) return orig_unlink ? orig_unlink(path) : unlink(path);
  int r = orig_unlink ? orig_unlink(path) : unlink(path);
  logFileIo("UNLINK", path, r, errno);
  return r;
}

static int hook_unlinkat(int fd, const char *path, int flag) {
  if (isCommentaryPath(path)) {
    ZLog(@"[BLV_PROTECT] Prevented deletion (unlinkat) of commentary file: %s", path);
    logFileIo("UNLINKAT_PROTECTED", path, 0, 0);
    return 0;
  }
  if (gInHook) return orig_unlinkat ? orig_unlinkat(fd, path, flag) : unlinkat(fd, path, flag);
  int r = orig_unlinkat ? orig_unlinkat(fd, path, flag) : unlinkat(fd, path, flag);
  logFileIo("UNLINKAT", path, r, errno);
  return r;
}

static int hook_remove(const char *path) {
  if (isCommentaryPath(path)) {
    ZLog(@"[BLV_PROTECT] Prevented deletion (remove) of commentary file: %s", path);
    logFileIo("REMOVE_PROTECTED", path, 0, 0);
    return 0;
  }
  if (gInHook) return orig_remove ? orig_remove(path) : remove(path);
  int r = orig_remove ? orig_remove(path) : remove(path);
  logFileIo("REMOVE", path, r, errno);
  return r;
}

static int hook_open(const char *path, int oflag, ...) {
  mode_t mode = 0;
  if (oflag & O_CREAT) {
    va_list ap;
    va_start(ap, oflag);
    mode = (mode_t)va_arg(ap, int);
    va_end(ap);
  }
  if (gInHook) {
    return orig_open ? orig_open(path, oflag, mode) : open(path, oflag, mode);
  }
  int fd = orig_open ? orig_open(path, oflag, mode) : open(path, oflag, mode);
  logFileIo("OPEN", path, fd >= 0 ? 0 : -1, errno);
  return fd;
}

static int hook_access(const char *path, int amode) {
  if (gInHook) return orig_access ? orig_access(path, amode) : access(path, amode);
  int r = orig_access ? orig_access(path, amode) : access(path, amode);
  logFileIo("ACCESS", path, r, errno);
  return r;
}

static int hook_stat(const char *path, struct stat *buf) {
  if (gInHook) return orig_stat ? orig_stat(path, buf) : stat(path, buf);
  int r = orig_stat ? orig_stat(path, buf) : stat(path, buf);
  logFileIo("STAT", path, r, errno);
  return r;
}

static int hook_lstat(const char *path, struct stat *buf) {
  if (gInHook) return orig_lstat ? orig_lstat(path, buf) : lstat(path, buf);
  int r = orig_lstat ? orig_lstat(path, buf) : lstat(path, buf);
  logFileIo("LSTAT", path, r, errno);
  return r;
}

// ---------------------------------------------------------------- NSFileManager swizzling
static IMP gOrigFmRemovePath, gOrigFmRemoveURL;

static BOOL hookFmRemovePath(id self, SEL _cmd, NSString* path, NSError** error) {
  if (path && isCommentaryPath(path.fileSystemRepresentation)) {
    ZLog(@"[BLV_PROTECT] Prevented NSFileManager removeItemAtPath: %@", path);
    logFileIo("NSFileManager.remove_PROTECTED", path.fileSystemRepresentation, 0, 0);
    return YES;
  }
  return ((BOOL(*)(id, SEL, NSString*, NSError**))gOrigFmRemovePath)(self, _cmd, path, error);
}

static BOOL hookFmRemoveURL(id self, SEL _cmd, NSURL* url, NSError** error) {
  if (url.path && isCommentaryPath(url.path.fileSystemRepresentation)) {
    ZLog(@"[BLV_PROTECT] Prevented NSFileManager removeItemAtURL: %@", url.path);
    logFileIo("NSFileManager.removeURL_PROTECTED", url.path.fileSystemRepresentation, 0, 0);
    return YES;
  }
  return ((BOOL(*)(id, SEL, NSURL*, NSError**))gOrigFmRemoveURL)(self, _cmd, url, error);
}

// ---------------------------------------------------------------- server
static std::unique_ptr<zsv::OfflineServer> gServer;
static std::once_flag gServerOnce;

static zsv::OfflineServer& server() {
  std::call_once(gServerOnce, [] {
    zsv::Config cfg;
    cfg.assetsDir = gBundleAssets.fileSystemRepresentation;
    std::string livedataDir;
    NSString* directLivedata = [gDocs stringByAppendingPathComponent:@"livedata"];
    NSString* subLivedata = [gDocs stringByAppendingPathComponent:@"PES2027-data/livedata"];
    NSFileManager* fm = NSFileManager.defaultManager;
    if ([fm fileExistsAtPath:directLivedata]) {
      livedataDir = directLivedata.fileSystemRepresentation;
    } else if ([fm fileExistsAtPath:subLivedata]) {
      livedataDir = subLivedata.fileSystemRepresentation;
    } else {
      std::string id = zd::activeId(layout());
      if (!id.empty()) livedataDir = zd::dataDir(layout(), id) + "/livedata";
    }
    cfg.livedataDir = livedataDir;
    cfg.dataDir = std::string(gSupport.fileSystemRepresentation) + "/Save";
    cfg.logFile = std::string([gDocs stringByAppendingPathComponent:@"PES2027-logs/offline_server.log"].fileSystemRepresentation);
    gServer.reset(new zsv::OfflineServer(cfg));
    gServer->startupAudit();
    ZLog(@"server ready assets=%s livedata=%s", cfg.assetsDir.c_str(), cfg.livedataDir.c_str());
  });
  return *gServer;
}

// ---------------------------------------------------------------- NSURLProtocol
static BOOL shouldHandleURL(NSURL* url) {
  NSString* host = url.host.lowercaseString;
  if (!host) return NO;
  NSString* scheme = url.scheme.lowercaseString;
  if (![scheme isEqualToString:@"http"] && ![scheme isEqualToString:@"https"]) return NO;
  if ([host isEqualToString:@"127.0.0.1"] && url.port.intValue == 8085) return YES;
  if ([host isEqualToString:@"ntl.service.konami.net"]) return YES;                 // GateInfo.php
  if ([host isEqualToString:@"d1ln4m3c7n87ju.cloudfront.net"]) return YES;          // livedata CDN in CmdLogin
  return NO;
}

@interface ZiruURLProtocol : NSURLProtocol
@end

@implementation ZiruURLProtocol
+ (BOOL)canInitWithRequest:(NSURLRequest*)request {
  if (shouldHandleURL(request.URL)) return YES;
  if (request.URL) {
    NSString* scheme = request.URL.scheme.lowercaseString;
    if ([scheme isEqualToString:@"http"] || [scheme isEqualToString:@"https"]) {
      ZLog(@"[REAL_NET] Unhandled network request: %@ %@", request.HTTPMethod ?: @"GET", request.URL.absoluteString);
    }
  }
  return NO;
}
+ (NSURLRequest*)canonicalRequestForRequest:(NSURLRequest*)request { return request; }
+ (BOOL)requestIsCacheEquivalent:(NSURLRequest*)a toRequest:(NSURLRequest*)b { return NO; }

static NSData* bodyOf(NSURLRequest* r) {
  if (r.HTTPBody) return r.HTTPBody;
  NSInputStream* s = r.HTTPBodyStream;
  if (!s) return nil;
  NSMutableData* d = [NSMutableData data];
  [s open];
  uint8_t buf[65536];
  while (true) {
    NSInteger n = [s read:buf maxLength:sizeof buf];
    if (n <= 0) break;
    [d appendBytes:buf length:(NSUInteger)n];
  }
  [s close];
  return d;
}

- (void)startLoading {
  NSURLRequest* rq = self.request;
  NSURL* url = rq.URL;
  zsv::Request r;
  r.method = rq.HTTPMethod ? rq.HTTPMethod.uppercaseString.UTF8String : "GET";
  NSString* path = url.path.length ? url.path : @"/";                // decoded, no query (NanoHTTPD getUri)
  if ([url.host.lowercaseString isEqualToString:@"d1ln4m3c7n87ju.cloudfront.net"]) {
    // CDN livedata URL -> /livedata/<file name> served from the imported pack
    path = [@"/livedata/" stringByAppendingString:url.lastPathComponent ?: @""];
  }
  r.uri = path.UTF8String;
  for (NSString* k in rq.allHTTPHeaderFields) r.headers[k.lowercaseString.UTF8String] = [rq.allHTTPHeaderFields[k] UTF8String];
  NSData* body = bodyOf(rq);
  if (body.length) {
    r.body.assign((const uint8_t*)body.bytes, (const uint8_t*)body.bytes + body.length);
    r.headers["content-length"] = std::to_string(body.length);  // NanoHTTPD reads exactly content-length
  } else {
    r.headers.erase("content-length");
  }
  CFAbsoluteTime t0 = CFAbsoluteTimeGetCurrent();
  zsv::Response resp = server().serve(r);
  double ms = (CFAbsoluteTimeGetCurrent() - t0) * 1000.0;
  ZLog(@"%s %s body=%lu -> %d %lu bytes (%.0f ms)", r.method.c_str(), r.uri.c_str(), (unsigned long)body.length, resp.status,
       (unsigned long)resp.body.size(), ms);
  NSMutableDictionary* h = [NSMutableDictionary dictionary];
  h[@"Content-Type"] = [NSString stringWithUTF8String:resp.mime.c_str()];
  h[@"Content-Length"] = [NSString stringWithFormat:@"%lu", (unsigned long)resp.body.size()];
  for (auto& kv : resp.headers) h[[NSString stringWithUTF8String:kv.first.c_str()]] = [NSString stringWithUTF8String:kv.second.c_str()];
  NSHTTPURLResponse* hr = [[NSHTTPURLResponse alloc] initWithURL:url statusCode:resp.status HTTPVersion:@"HTTP/1.1" headerFields:h];
  [self.client URLProtocol:self didReceiveResponse:hr cacheStoragePolicy:NSURLCacheStorageNotAllowed];
  if (!resp.body.empty()) [self.client URLProtocol:self didLoadData:[NSData dataWithBytes:resp.body.data() length:resp.body.size()]];
  [self.client URLProtocolDidFinishLoading:self];
}
- (void)stopLoading {}
@end

// ---------------------------------------------------------------- swizzling helpers
static IMP swizzleClassMethod(Class c, SEL s, IMP repl) {
  Method m = class_getClassMethod(c, s);
  if (!m) return nullptr;
  return method_setImplementation(m, repl);
}
static IMP swizzleInstance(Class c, SEL s, IMP repl) {
  Method m = class_getInstanceMethod(c, s);
  if (!m) return nullptr;
  if (!class_addMethod(c, s, repl, method_getTypeEncoding(m))) return method_setImplementation(m, repl);
  return method_getImplementation(m);
}

// NSURLSessionConfiguration: put our protocol first in every default/ephemeral configuration
static IMP gOrigDefaultCfg, gOrigEphemeralCfg;
static NSURLSessionConfiguration* withProtocol(NSURLSessionConfiguration* c) {
  NSMutableArray* p = [NSMutableArray arrayWithArray:c.protocolClasses ?: @[]];
  if (![p containsObject:ZiruURLProtocol.class]) [p insertObject:ZiruURLProtocol.class atIndex:0];
  c.protocolClasses = p;
  return c;
}
static NSURLSessionConfiguration* hookDefaultCfg(id self, SEL _cmd) {
  return withProtocol(((NSURLSessionConfiguration * (*)(id, SEL)) gOrigDefaultCfg)(self, _cmd));
}
static NSURLSessionConfiguration* hookEphemeralCfg(id self, SEL _cmd) {
  return withProtocol(((NSURLSessionConfiguration * (*)(id, SEL)) gOrigEphemeralCfg)(self, _cmd));
}

// -[HttpImpl*Request setRequest:(const char*)url cookie:(const char*) isPost:(BOOL) sendData:(const void*) sendDataSize:(int) useGzip:(BOOL)]
typedef void (*SetReqFn)(id, SEL, const char*, const char*, long, const void*, long, long);
static SetReqFn gOrigSetReq[3];
template <int I>
static void hookSetRequest(id self, SEL _cmd, const char* url, const char* cookie, long isPost, const void* data, long size, long gzip) {
  std::string orig = url ? url : "";
  std::string red = url ? zsv::OfflineServer::redirectUrl(orig) : orig;
  if (red != orig) {
    ZLog(@"[REDIRECT] %s -> %s", orig.c_str(), red.c_str());
  } else {
    ZLog(@"[HTTP_DIRECT] %s", orig.c_str());
  }
  gOrigSetReq[I](self, _cmd, url ? red.c_str() : url, cookie, isPost, data, size, gzip);
}

// ---------------------------------------------------------------- launch gating
static std::atomic<bool> gGameStarted{false};
static IMP gOrigDidFinish, gOrigBecomeActive, gOrigResignActive, gOrigEnterBg, gOrigEnterFg;
static __strong id gLaunchApp;
static __strong NSDictionary* gLaunchOptions;
static __strong id gDelegate;
static __strong UIWindow* gImportWindow;
static void startGame();

static BOOL checkDirectFolderData(NSString** why) {
  NSFileManager* fm = NSFileManager.defaultManager;

  // Case 2: User copied a single folder like "PES2027-data" into Documents
  NSString* subfolder = nil;
  for (NSString* item in [fm contentsOfDirectoryAtPath:gDocs error:nil]) {
    NSString* full = [gDocs stringByAppendingPathComponent:item];
    BOOL isDir = NO;
    if ([fm fileExistsAtPath:full isDirectory:&isDir] && isDir) {
      if ([item.lowercaseString containsString:@"pes2027-data"] || [item.lowercaseString isEqualToString:@"data"]) {
        NSString* subPak = [full stringByAppendingPathComponent:@"pak"];
        NSString* subCpk = [full stringByAppendingPathComponent:@"cpk"];
        if ([fm fileExistsAtPath:subPak] && [fm fileExistsAtPath:subCpk]) {
          subfolder = full;
          break;
        }
      }
    }
  }

  if (subfolder) {
    ZLog(@"Found data subfolder: %@", subfolder);
    for (NSString* name in @[@"pak", @"cpk"]) {
      NSString* link = [gDocs stringByAppendingPathComponent:name];
      NSString* target = [subfolder stringByAppendingPathComponent:name];
      NSDictionary* attr = [fm attributesOfItemAtPath:link error:nil];
      if (attr) {
        if ([attr[NSFileType] isEqualToString:NSFileTypeSymbolicLink]) {
          [fm removeItemAtPath:link error:nil];
          [fm createSymbolicLinkAtPath:link withDestinationPath:target error:nil];
        }
      } else {
        [fm createSymbolicLinkAtPath:link withDestinationPath:target error:nil];
      }
    }
    NSString* subDl = [subfolder stringByAppendingPathComponent:@"download"];
    if (![fm fileExistsAtPath:subDl]) subDl = [subfolder stringByAppendingPathComponent:@"Download"];
    if ([fm fileExistsAtPath:subDl]) {
      NSString* dlDir = [gDocs stringByAppendingPathComponent:@"Download"];
      [fm createDirectoryAtPath:dlDir withIntermediateDirectories:YES attributes:nil error:nil];
      for (NSString* f in [fm contentsOfDirectoryAtPath:subDl error:nil]) {
        NSString* src = [subDl stringByAppendingPathComponent:f];
        NSString* dst = [dlDir stringByAppendingPathComponent:f];
        if (![fm fileExistsAtPath:dst]) {
          [fm createSymbolicLinkAtPath:dst withDestinationPath:src error:nil];
        }
      }
    }
  }

  // Case 1: Direct folders pak and cpk in Documents
  NSString* pakDir = [gDocs stringByAppendingPathComponent:@"pak"];
  NSString* cpkDir = [gDocs stringByAppendingPathComponent:@"cpk"];
  BOOL isDir = NO;
  if (![fm fileExistsAtPath:pakDir isDirectory:&isDir]) return NO;
  if (![fm fileExistsAtPath:cpkDir isDirectory:&isDir]) return NO;

  NSArray* pakItems = [fm contentsOfDirectoryAtPath:pakDir error:nil];
  NSArray* cpkItems = [fm contentsOfDirectoryAtPath:cpkDir error:nil];
  if (pakItems.count < 3 || cpkItems.count < 3) {
    if (why) *why = @"Thư mục pak hoặc cpk trong Documents thiếu file dữ liệu";
    return NO;
  }

  // Ensure Download folder exists
  NSString* dlDir = [gDocs stringByAppendingPathComponent:@"Download"];
  [fm createDirectoryAtPath:dlDir withIntermediateDirectories:YES attributes:nil error:nil];

  ZLog(@"Direct folder data ready: pak=%lu files, cpk=%lu files", (unsigned long)pakItems.count, (unsigned long)cpkItems.count);
  return YES;
}

static BOOL dataReady(NSString** why) {
  if (checkDirectFolderData(why)) return YES;
  std::string err;
  zd::Layout L = layout();
  std::string id = zd::activeId(L);
  NSString* appVer = [NSBundle.mainBundle.infoDictionary objectForKey:@"CFBundleShortVersionString"] ?: @"";
  NSString* appBuild = [NSBundle.mainBundle.infoDictionary objectForKey:@"CFBundleVersion"] ?: @"";
  bool ok = zd::quickVerify(L, id, appVer.UTF8String ?: "", appBuild.UTF8String ?: "", ZIRU_MOD_VERSION, err);
  if (!ok && why) *why = [NSString stringWithUTF8String:err.c_str()];
  return ok;
}

// ---------------------------------------------------------------- PES21 UI components
@interface PESPressButton : UIButton
@end

@implementation PESPressButton
- (void)setHighlighted:(BOOL)highlighted {
  [super setHighlighted:highlighted];
  [UIView animateWithDuration:0.22 delay:0 usingSpringWithDamping:0.75 initialSpringVelocity:0.5
                      options:UIViewAnimationOptionBeginFromCurrentState | UIViewAnimationOptionAllowUserInteraction
                   animations:^{
                     self.transform = highlighted ? CGAffineTransformMakeScale(0.97, 0.97) : CGAffineTransformIdentity;
                   } completion:nil];
}
@end

@interface PESPitchBackdrop : UIView
@property (nonatomic, strong) CAGradientLayer* gradient;
@property (nonatomic, strong) CAShapeLayer* pitch;
@property (nonatomic, strong) CAShapeLayer* slash;
@end

@implementation PESPitchBackdrop
- (instancetype)initWithFrame:(CGRect)frame {
  if ((self = [super initWithFrame:frame])) {
    self.userInteractionEnabled = NO;
    _gradient = [CAGradientLayer layer];
    _gradient.colors = @[
      (id)PESNight().CGColor,
      (id)[UIColor colorWithRed:0.035 green:0.08 blue:0.11 alpha:1.0].CGColor,
      (id)PESNight().CGColor
    ];
    _gradient.startPoint = CGPointMake(0.0, 0.0);
    _gradient.endPoint = CGPointMake(1.0, 1.0);
    [self.layer addSublayer:_gradient];

    _slash = [CAShapeLayer layer];
    _slash.fillColor = [PESLime() colorWithAlphaComponent:0.035].CGColor;
    [self.layer addSublayer:_slash];

    _pitch = [CAShapeLayer layer];
    _pitch.strokeColor = [PESCyan() colorWithAlphaComponent:0.12].CGColor;
    _pitch.fillColor = UIColor.clearColor.CGColor;
    _pitch.lineWidth = 1.2;
    [self.layer addSublayer:_pitch];
  }
  return self;
}

- (void)layoutSubviews {
  [super layoutSubviews];
  _gradient.frame = self.bounds;
  CGFloat w = self.bounds.size.width;
  CGFloat h = self.bounds.size.height;
  if (w <= 0 || h <= 0) return;

  UIBezierPath* sp = [UIBezierPath bezierPath];
  [sp moveToPoint:CGPointMake(w * 0.18, 0)];
  [sp addLineToPoint:CGPointMake(w * 0.48, 0)];
  [sp addLineToPoint:CGPointMake(w * 0.32, h)];
  [sp addLineToPoint:CGPointMake(0, h)];
  [sp closePath];
  _slash.path = sp.CGPath;

  UIBezierPath* p = [UIBezierPath bezierPath];
  CGFloat pad = 24.0;
  CGRect pitchRect = CGRectInset(self.bounds, pad, pad);
  [p appendPath:[UIBezierPath bezierPathWithRoundedRect:pitchRect cornerRadius:14.0]];
  CGFloat midX = CGRectGetMidX(pitchRect);
  [p moveToPoint:CGPointMake(midX, CGRectGetMinY(pitchRect))];
  [p addLineToPoint:CGPointMake(midX, CGRectGetMaxY(pitchRect))];
  CGPoint center = CGPointMake(midX, CGRectGetMidY(pitchRect));
  [p moveToPoint:CGPointMake(center.x + 65.0, center.y)];
  [p addArcWithCenter:center radius:65.0 startAngle:0 endAngle:M_PI * 2 clockwise:YES];
  _pitch.path = p.CGPath;
}
@end

// ---------------------------------------------------------------- diagnostics modal (log viewer)
@interface PESDiagnosticsVC : UIViewController
@property (nonatomic, strong) UISegmentedControl* seg;
@property (nonatomic, strong) UITextView* tv;
@property (nonatomic, strong) NSArray<NSString*>* logNames;
@end

@implementation PESDiagnosticsVC
- (UIInterfaceOrientationMask)supportedInterfaceOrientations { return UIInterfaceOrientationMaskLandscape; }
- (BOOL)prefersStatusBarHidden { return YES; }

- (void)viewDidLoad {
  [super viewDidLoad];
  self.view.backgroundColor = PESNight();
  self.logNames = @[@"ziru.log", @"fileio.log", @"crash.log", @"offline_server.log"];

  UIView* topBar = [UIView new];
  topBar.translatesAutoresizingMaskIntoConstraints = NO;
  [self.view addSubview:topBar];

  UIButton* closeBtn = [UIButton buttonWithType:UIButtonTypeSystem];
  [closeBtn setTitle:@"Đóng" forState:UIControlStateNormal];
  closeBtn.titleLabel.font = [UIFont boldSystemFontOfSize:16];
  closeBtn.tintColor = PESLime();
  [closeBtn addTarget:self action:@selector(onClose) forControlEvents:UIControlEventTouchUpInside];
  closeBtn.translatesAutoresizingMaskIntoConstraints = NO;
  [topBar addSubview:closeBtn];

  UIButton* shareBtn = [UIButton buttonWithType:UIButtonTypeSystem];
  [shareBtn setTitle:@"Chia sẻ nhật ký" forState:UIControlStateNormal];
  shareBtn.titleLabel.font = [UIFont boldSystemFontOfSize:16];
  shareBtn.tintColor = PESCyan();
  [shareBtn addTarget:self action:@selector(onShare) forControlEvents:UIControlEventTouchUpInside];
  shareBtn.translatesAutoresizingMaskIntoConstraints = NO;
  [topBar addSubview:shareBtn];

  self.seg = [[UISegmentedControl alloc] initWithItems:@[@"Ziru", @"File I/O", @"Crash", @"Server"]];
  self.seg.selectedSegmentIndex = 0;
  self.seg.selectedSegmentTintColor = [PESLime() colorWithAlphaComponent:0.25];
  [self.seg setTitleTextAttributes:@{NSForegroundColorAttributeName: PESLime()} forState:UIControlStateSelected];
  [self.seg setTitleTextAttributes:@{NSForegroundColorAttributeName: UIColor.whiteColor} forState:UIControlStateNormal];
  [self.seg addTarget:self action:@selector(onTabChanged) forControlEvents:UIControlEventValueChanged];
  self.seg.translatesAutoresizingMaskIntoConstraints = NO;
  [topBar addSubview:self.seg];

  self.tv = [UITextView new];
  self.tv.editable = NO;
  self.tv.backgroundColor = [UIColor colorWithWhite:0 alpha:0.4];
  self.tv.layer.cornerRadius = 10;
  self.tv.font = [UIFont fontWithName:@"Courier" size:11] ?: [UIFont monospacedSystemFontOfSize:11 weight:UIFontWeightRegular];
  self.tv.textColor = [UIColor colorWithRed:0.8 green:0.9 blue:0.95 alpha:1.0];
  self.tv.translatesAutoresizingMaskIntoConstraints = NO;
  [self.view addSubview:self.tv];

  UILayoutGuide* g = self.view.safeAreaLayoutGuide;
  [NSLayoutConstraint activateConstraints:@[
    [topBar.topAnchor constraintEqualToAnchor:g.topAnchor constant:8],
    [topBar.leadingAnchor constraintEqualToAnchor:g.leadingAnchor constant:16],
    [topBar.trailingAnchor constraintEqualToAnchor:g.trailingAnchor constant:-16],
    [topBar.heightAnchor constraintEqualToConstant:40],

    [closeBtn.leadingAnchor constraintEqualToAnchor:topBar.leadingAnchor],
    [closeBtn.centerYAnchor constraintEqualToAnchor:topBar.centerYAnchor],

    [self.seg.centerXAnchor constraintEqualToAnchor:topBar.centerXAnchor],
    [self.seg.centerYAnchor constraintEqualToAnchor:topBar.centerYAnchor],

    [shareBtn.trailingAnchor constraintEqualToAnchor:topBar.trailingAnchor],
    [shareBtn.centerYAnchor constraintEqualToAnchor:topBar.centerYAnchor],

    [self.tv.topAnchor constraintEqualToAnchor:topBar.bottomAnchor constant:12],
    [self.tv.leadingAnchor constraintEqualToAnchor:g.leadingAnchor constant:16],
    [self.tv.trailingAnchor constraintEqualToAnchor:g.trailingAnchor constant:-16],
    [self.tv.bottomAnchor constraintEqualToAnchor:g.bottomAnchor constant:-12],
  ]];

  [self onTabChanged];
}

- (void)onClose {
  [self dismissViewControllerAnimated:YES completion:nil];
}

- (void)onTabChanged {
  NSInteger idx = self.seg.selectedSegmentIndex;
  if (idx < 0 || idx >= (NSInteger)self.logNames.count) return;
  NSString* name = self.logNames[idx];
  NSString* path = [gDocs stringByAppendingPathComponent:[NSString stringWithFormat:@"PES2027-logs/%@", name]];
  NSError* err = nil;
  NSString* content = [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:&err];
  if (!content || !content.length) {
    self.tv.text = [NSString stringWithFormat:@"[Chưa có dữ liệu trong %@]", name];
  } else {
    self.tv.text = content;
    if (content.length > 0) {
      [self.tv scrollRangeToVisible:NSMakeRange(content.length - 1, 1)];
    }
  }
}

- (void)onShare {
  NSMutableArray* urls = [NSMutableArray array];
  NSString* dir = [gDocs stringByAppendingPathComponent:@"PES2027-logs"];
  for (NSString* n in self.logNames) {
    NSString* p = [dir stringByAppendingPathComponent:n];
    if ([NSFileManager.defaultManager fileExistsAtPath:p]) {
      [urls addObject:[NSURL fileURLWithPath:p]];
    }
  }
  if (!urls.count) {
    UIAlertController* a = [UIAlertController alertControllerWithTitle:@"Chưa có nhật ký" message:@"Chưa có file log nào để chia sẻ." preferredStyle:UIAlertControllerStyleAlert];
    [a addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleCancel handler:nil]];
    [self presentViewController:a animated:YES completion:nil];
    return;
  }
  UIActivityViewController* act = [[UIActivityViewController alloc] initWithActivityItems:urls applicationActivities:nil];
  act.popoverPresentationController.sourceView = self.view;
  [self presentViewController:act animated:YES completion:nil];
}
@end

// ---------------------------------------------------------------- ZiruImportVC
@interface ZiruImportVC : UIViewController <UIDocumentPickerDelegate>
@property(nonatomic, strong) UILabel* status;
@property(nonatomic, strong) UILabel* detail;
@property(nonatomic, strong) UIProgressView* bar;
@property(nonatomic, strong) PESPressButton* pick;
@property(nonatomic, strong) PESPressButton* local;
@property(nonatomic, strong) PESPressButton* rescan;
@property(nonatomic, strong) PESPressButton* logs;
@property(nonatomic, strong) UILabel* watermarkLabel;
@property(nonatomic, copy) NSString* localZip;
@property(atomic) BOOL busy;
@end

@implementation ZiruImportVC
- (UIInterfaceOrientationMask)supportedInterfaceOrientations { return UIInterfaceOrientationMaskLandscape; }
- (BOOL)prefersStatusBarHidden { return YES; }

static PESPressButton* makeButton(NSString* title, BOOL primary) {
  PESPressButton* b = [PESPressButton buttonWithType:UIButtonTypeCustom];
  [b setTitle:title forState:UIControlStateNormal];
  b.titleLabel.font = [UIFont boldSystemFontOfSize:16];
  if (primary) {
    b.backgroundColor = PESLime();
    [b setTitleColor:PESNight() forState:UIControlStateNormal];
  } else {
    b.backgroundColor = [UIColor colorWithWhite:1 alpha:0.07];
    b.layer.borderWidth = 1.2;
    b.layer.borderColor = [PESLime() colorWithAlphaComponent:0.4].CGColor;
    [b setTitleColor:PESLime() forState:UIControlStateNormal];
  }
  b.layer.cornerRadius = 12;
  b.contentEdgeInsets = UIEdgeInsetsMake(12, 24, 12, 24);
  b.translatesAutoresizingMaskIntoConstraints = NO;
  return b;
}

- (void)checkTamper {
  // Enforce unremovable watermark "Made by iOSVN"
  if (self.watermarkLabel) {
    if (self.watermarkLabel.hidden || self.watermarkLabel.alpha < 0.1 ||
        ![self.watermarkLabel.text containsString:@"Made by iOSVN"]) {
      NSLog(@"[CRITICAL ERROR] Integrity failure: Watermark Made by iOSVN was modified or removed.");
      abort();
    }
  }
}

- (void)viewDidLoad {
  [super viewDidLoad];

  // Pitch backdrop (PES21 stadium style)
  PESPitchBackdrop* backdrop = [[PESPitchBackdrop alloc] initWithFrame:self.view.bounds];
  backdrop.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  [self.view addSubview:backdrop];

  // Glass Card container
  UIView* card = [UIView new];
  card.backgroundColor = [UIColor colorWithRed:0.035 green:0.065 blue:0.095 alpha:0.92];
  card.layer.cornerRadius = 20;
  card.layer.borderWidth = 1.4;
  card.layer.borderColor = [PESCyan() colorWithAlphaComponent:0.25].CGColor;
  card.translatesAutoresizingMaskIntoConstraints = NO;
  [self.view addSubview:card];

  // Title Header: "PES 2027" + "OFFLINE"
  NSMutableAttributedString* mat = [[NSMutableAttributedString alloc]
    initWithString:@"PES 2027 "
    attributes:@{
      NSFontAttributeName: [UIFont italicSystemFontOfSize:26],
      NSForegroundColorAttributeName: UIColor.whiteColor
    }];
  [mat appendAttributedString:[[NSAttributedString alloc]
    initWithString:@"OFFLINE"
    attributes:@{
      NSFontAttributeName: [UIFont boldSystemFontOfSize:26],
      NSForegroundColorAttributeName: PESLime()
    }]];
  UILabel* title = [UILabel new];
  title.attributedText = mat;
  title.textAlignment = NSTextAlignmentCenter;

  // Subtitle Badge Pill
  UIView* pill = [UIView new];
  pill.backgroundColor = [PESLime() colorWithAlphaComponent:0.08];
  pill.layer.cornerRadius = 11;
  pill.layer.borderWidth = 1;
  pill.layer.borderColor = [PESLime() colorWithAlphaComponent:0.28].CGColor;
  UILabel* pillText = [UILabel new];
  pillText.text = @"eFootball Ziru • iOS Mod v" ZIRU_MOD_VERSION;
  pillText.font = [UIFont systemFontOfSize:12 weight:UIFontWeightMedium];
  pillText.textColor = PESLime();
  pillText.translatesAutoresizingMaskIntoConstraints = NO;
  [pill addSubview:pillText];
  [NSLayoutConstraint activateConstraints:@[
    [pillText.topAnchor constraintEqualToAnchor:pill.topAnchor constant:4],
    [pillText.bottomAnchor constraintEqualToAnchor:pill.bottomAnchor constant:-4],
    [pillText.leadingAnchor constraintEqualToAnchor:pill.leadingAnchor constant:12],
    [pillText.trailingAnchor constraintEqualToAnchor:pill.trailingAnchor constant:-12],
  ]];

  self.status = [UILabel new];
  self.status.textColor = [UIColor colorWithWhite:0.92 alpha:1];
  self.status.font = [UIFont systemFontOfSize:15 weight:UIFontWeightRegular];
  self.status.numberOfLines = 0;
  self.status.textAlignment = NSTextAlignmentCenter;

  self.detail = [UILabel new];
  self.detail.textColor = PESMuted();
  self.detail.font = [UIFont systemFontOfSize:12];
  self.detail.numberOfLines = 0;
  self.detail.textAlignment = NSTextAlignmentCenter;

  self.bar = [[UIProgressView alloc] initWithProgressViewStyle:UIProgressViewStyleDefault];
  self.bar.progressTintColor = PESLime();
  self.bar.trackTintColor = [UIColor colorWithWhite:1 alpha:0.12];
  self.bar.layer.cornerRadius = 3;
  self.bar.clipsToBounds = YES;
  self.bar.hidden = YES;

  self.pick = makeButton(@"Chọn file (.zip) từ Tệp", YES);
  [self.pick addTarget:self action:@selector(onPick) forControlEvents:UIControlEventTouchUpInside];

  self.local = makeButton(@"Nhập file .zip trong Documents", NO);
  [self.local addTarget:self action:@selector(onLocal) forControlEvents:UIControlEventTouchUpInside];

  self.rescan = makeButton(@"Quét thư mục data (3uTools)", NO);
  [self.rescan setTitleColor:PESLime() forState:UIControlStateNormal];
  self.rescan.layer.borderColor = [PESLime() colorWithAlphaComponent:0.6].CGColor;
  [self.rescan addTarget:self action:@selector(onRescan) forControlEvents:UIControlEventTouchUpInside];

  self.logs = makeButton(@"Xem nhật ký (Logs)", NO);
  self.logs.titleLabel.font = [UIFont systemFontOfSize:13 weight:UIFontWeightMedium];
  self.logs.contentEdgeInsets = UIEdgeInsetsMake(6, 16, 6, 16);
  [self.logs setTitleColor:PESCyan() forState:UIControlStateNormal];
  self.logs.layer.borderColor = [PESCyan() colorWithAlphaComponent:0.35].CGColor;
  [self.logs addTarget:self action:@selector(onShowLogs) forControlEvents:UIControlEventTouchUpInside];

  // Watermark "Made by iOSVN"
  UIView* wmContainer = [UIView new];
  wmContainer.backgroundColor = [PESLime() colorWithAlphaComponent:0.07];
  wmContainer.layer.cornerRadius = 10;
  wmContainer.layer.borderWidth = 1;
  wmContainer.layer.borderColor = [PESLime() colorWithAlphaComponent:0.3].CGColor;
  wmContainer.translatesAutoresizingMaskIntoConstraints = NO;

  self.watermarkLabel = [UILabel new];
  self.watermarkLabel.text = @"⚡ Made by iOSVN";
  self.watermarkLabel.font = [UIFont systemFontOfSize:12 weight:UIFontWeightBold];
  self.watermarkLabel.textColor = PESLime();
  self.watermarkLabel.textAlignment = NSTextAlignmentCenter;
  self.watermarkLabel.translatesAutoresizingMaskIntoConstraints = NO;
  [wmContainer addSubview:self.watermarkLabel];

  [NSLayoutConstraint activateConstraints:@[
    [self.watermarkLabel.topAnchor constraintEqualToAnchor:wmContainer.topAnchor constant:4],
    [self.watermarkLabel.bottomAnchor constraintEqualToAnchor:wmContainer.bottomAnchor constant:-4],
    [self.watermarkLabel.leadingAnchor constraintEqualToAnchor:wmContainer.leadingAnchor constant:14],
    [self.watermarkLabel.trailingAnchor constraintEqualToAnchor:wmContainer.trailingAnchor constant:-14],
  ]];

  UIStackView* btnStack = [[UIStackView alloc] initWithArrangedSubviews:@[self.rescan, self.pick, self.local]];
  btnStack.axis = UILayoutConstraintAxisHorizontal;
  btnStack.alignment = UIStackViewAlignmentCenter;
  btnStack.spacing = 10;

  UIStackView* cardStack = [[UIStackView alloc] initWithArrangedSubviews:@[
    title, pill, self.status, self.bar, btnStack, self.detail, self.logs, wmContainer
  ]];
  cardStack.axis = UILayoutConstraintAxisVertical;
  cardStack.alignment = UIStackViewAlignmentCenter;
  cardStack.spacing = 13;
  cardStack.translatesAutoresizingMaskIntoConstraints = NO;
  [card addSubview:cardStack];

  UILayoutGuide* g = self.view.safeAreaLayoutGuide;
  [NSLayoutConstraint activateConstraints:@[
    [card.centerXAnchor constraintEqualToAnchor:g.centerXAnchor],
    [card.centerYAnchor constraintEqualToAnchor:g.centerYAnchor],
    [card.widthAnchor constraintLessThanOrEqualToAnchor:g.widthAnchor multiplier:0.92],
    [cardStack.topAnchor constraintEqualToAnchor:card.topAnchor constant:22],
    [cardStack.bottomAnchor constraintEqualToAnchor:card.bottomAnchor constant:-18],
    [cardStack.leadingAnchor constraintEqualToAnchor:card.leadingAnchor constant:24],
    [cardStack.trailingAnchor constraintEqualToAnchor:card.trailingAnchor constant:-24],
    [self.bar.widthAnchor constraintEqualToConstant:420],
  ]];

  [self refresh];
}

- (void)viewDidAppear:(BOOL)animated {
  [super viewDidAppear:animated];
  [self checkTamper];
}

- (void)onShowLogs {
  [self checkTamper];
  PESDiagnosticsVC* vc = [PESDiagnosticsVC new];
  vc.modalPresentationStyle = UIModalPresentationFullScreen;
  [self presentViewController:vc animated:YES completion:nil];
}

- (NSString*)findLocalZip {
  NSArray* items = [NSFileManager.defaultManager contentsOfDirectoryAtPath:gDocs error:nil];
  for (NSString* n in [items sortedArrayUsingSelector:@selector(compare:)]) {
    if ([n.lowercaseString hasPrefix:@"pes2027-data"] && [n.lowercaseString hasSuffix:@".zip"]) return [gDocs stringByAppendingPathComponent:n];
  }
  return nil;
}

- (void)refresh {
  NSString* why = nil;
  if (dataReady(&why)) {
    self.status.text = @"Đã phát hiện dữ liệu game! Đang vào game…";
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.4 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
      startGame();
    });
    return;
  }
  self.localZip = [self findLocalZip];
  self.local.hidden = self.localZip == nil;
  if (self.localZip) [self.local setTitle:[NSString stringWithFormat:@"Nhập %@", self.localZip.lastPathComponent] forState:UIControlStateNormal];
  self.status.text = @"Chưa có dữ liệu game. Bạn có thể:\n"
                     @"1. Dùng 3uTools chép thẳng thư mục data (pak, cpk, Download, livedata) vào app.\n"
                     @"2. Hoặc chọn file .zip từ app Tệp để giải nén.";
  self.detail.text = [NSString stringWithFormat:@"Bản mod %s • %@", ZIRU_MOD_VERSION, why ?: @""];
}

- (void)onRescan {
  [self checkTamper];
  NSString* why = nil;
  if (dataReady(&why)) {
    self.status.text = @"Đã phát hiện dữ liệu game! Đang khởi động…";
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.4 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
      startGame();
    });
  } else {
    [self refresh];
    UIAlertController* a = [UIAlertController alertControllerWithTitle:@"Chưa thấy đủ thư mục dữ liệu"
                                                               message:why ?: @"Hãy đảm bảo đã chép các thư mục pak, cpk, Download, livedata vào Documents."
                                                        preferredStyle:UIAlertControllerStyleAlert];
    [a addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];
    [self presentViewController:a animated:YES completion:nil];
  }
}

- (void)onPick {
  [self checkTamper];
  if (self.busy) return;
  UIDocumentPickerViewController* p;
  if (@available(iOS 14.0, *)) {
    p = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[ UTTypeZIP, UTTypeData ] asCopy:NO];
  } else {
    p = [[UIDocumentPickerViewController alloc] initWithDocumentTypes:@[ @"public.zip-archive", @"public.data" ] inMode:UIDocumentPickerModeOpen];
  }
  p.delegate = self;
  p.allowsMultipleSelection = NO;
  [self presentViewController:p animated:YES completion:nil];
}

- (void)documentPicker:(UIDocumentPickerViewController*)controller didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls {
  NSURL* u = urls.firstObject;
  if (!u) return;
  BOOL scoped = [u startAccessingSecurityScopedResource];
  [self importPath:u.path done:^{
    if (scoped) [u stopAccessingSecurityScopedResource];
  }];
}

- (void)onLocal {
  [self checkTamper];
  if (self.busy || !self.localZip) return;
  [self importPath:self.localZip done:nil];
}

static uint64_t freeBytes() {
  NSURL* u = [NSURL fileURLWithPath:gSupport];
  NSNumber* n = nil;
  if (@available(iOS 11.0, *)) [u getResourceValue:&n forKey:NSURLVolumeAvailableCapacityForImportantUsageKey error:nil];
  if (!n) {
    NSDictionary* a = [NSFileManager.defaultManager attributesOfFileSystemForPath:gSupport error:nil];
    n = a[NSFileSystemFreeSize];
  }
  return n.unsignedLongLongValue;
}

- (void)importPath:(NSString*)path done:(void (^)(void))done {
  [self checkTamper];
  self.busy = YES;
  self.pick.enabled = NO;
  self.local.enabled = NO;
  self.bar.hidden = NO;
  self.bar.progress = 0;
  self.status.text = @"Đang kiểm tra gói data…";
  UIApplication.sharedApplication.idleTimerDisabled = YES;
  std::string zip = path.fileSystemRepresentation;
  dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
    __block UIBackgroundTaskIdentifier bg = [UIApplication.sharedApplication beginBackgroundTaskWithExpirationHandler:^{
      [UIApplication.sharedApplication endBackgroundTask:bg];
    }];
    std::string err;
    zd::Manifest m;
    NSString* ver = NSBundle.mainBundle.infoDictionary[@"CFBundleShortVersionString"];
    NSString* build = NSBundle.mainBundle.infoDictionary[@"CFBundleVersion"];
    bool ok = zd::inspectPack(zip, m, err) && zd::checkCompatible(m, ver.UTF8String ?: "", build.UTF8String ?: "", ZIRU_MOD_VERSION, err);
    if (ok) {
      CFAbsoluteTime last = 0;
      ok = zd::installPack(zip, layout(), freeBytes(), [&](const zd::Progress& p) {
        CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();
        if (now - last < 0.2 && p.phase == "extract") return true;
        last = now;
        float f = p.total ? (float)((double)p.done / (double)p.total) : 0;
        NSString* phase = p.phase == "extract" ? @"Đang giải nén và kiểm tra SHA-256" : p.phase == "activate" ? @"Đang kích hoạt" : @"Đang kiểm tra";
        NSString* txt = [NSString stringWithFormat:@"%@\n%s\n%.0f / %.0f MB", phase, p.file.c_str(), p.done / 1048576.0, p.total / 1048576.0];
        dispatch_async(dispatch_get_main_queue(), ^{
          self.bar.progress = f;
          self.status.text = txt;
        });
        return true;
      }, m, err);
    }
    NSString* msg = [NSString stringWithUTF8String:err.c_str()];
    ZLog(@"import %s: %@ (data_id=%s)", ok ? "OK" : "FAILED", msg, m.dataId.c_str());
    dispatch_async(dispatch_get_main_queue(), ^{
      if (done) done();
      UIApplication.sharedApplication.idleTimerDisabled = NO;
      [UIApplication.sharedApplication endBackgroundTask:bg];
      self.busy = NO;
      self.pick.enabled = YES;
      self.local.enabled = YES;
      if (ok) {
        self.status.text = @"Đã nhập xong dữ liệu. Đang vào game…";
        UIAlertController* a = [UIAlertController alertControllerWithTitle:@"Hoàn tất"
                                                                   message:@"Dữ liệu đã nhập thành công. Có thể xóa file .zip gốc để giải phóng dung lượng."
                                                            preferredStyle:UIAlertControllerStyleAlert];
        [a addAction:[UIAlertAction actionWithTitle:@"Vào game" style:UIAlertActionStyleDefault handler:^(UIAlertAction*) { startGame(); }]];
        [self presentViewController:a animated:YES completion:nil];
      } else {
        self.bar.hidden = YES;
        UIAlertController* a = [UIAlertController alertControllerWithTitle:@"Nhập dữ liệu thất bại" message:msg preferredStyle:UIAlertControllerStyleAlert];
        [a addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];
        [self presentViewController:a animated:YES completion:nil];
        [self refresh];
      }
    });
  });
}
@end

static void startGame() {
  if (gGameStarted.exchange(true)) return;
  ZLog(@"starting UE4 (didFinishLaunching)");
  if (gImportWindow) {
    gImportWindow.hidden = YES;
    gImportWindow.rootViewController = nil;
    gImportWindow = nil;
  }
  ((BOOL(*)(id, SEL, id, id))gOrigDidFinish)(gDelegate, @selector(application:didFinishLaunchingWithOptions:), gLaunchApp, gLaunchOptions);
  if (UIApplication.sharedApplication.applicationState == UIApplicationStateActive && gOrigBecomeActive)
    ((void (*)(id, SEL, id))gOrigBecomeActive)(gDelegate, @selector(applicationDidBecomeActive:), gLaunchApp);
}

static BOOL hookDidFinish(id self, SEL _cmd, UIApplication* app, NSDictionary* opts) {
  gDelegate = self;
  gLaunchApp = app;
  gLaunchOptions = opts;
  NSString* why = nil;
  if (dataReady(&why)) {
    ZLog(@"data ready (%s), launching directly", zd::activeId(layout()).c_str());
    gGameStarted = true;
    return ((BOOL(*)(id, SEL, id, id))gOrigDidFinish)(self, _cmd, app, opts);
  }
  ZLog(@"data not ready: %@ — showing importer", why);
  gImportWindow = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
  gImportWindow.rootViewController = [ZiruImportVC new];
  [gImportWindow makeKeyAndVisible];
  return YES;
}

#define GATED(NAME, ORIG)                                                  \
  static void NAME(id self, SEL _cmd, UIApplication* app) {                \
    if (!gGameStarted) return;                                             \
    if (ORIG) ((void (*)(id, SEL, id))ORIG)(self, _cmd, app);              \
  }
GATED(hookBecomeActive, gOrigBecomeActive)
GATED(hookResignActive, gOrigResignActive)
GATED(hookEnterBg, gOrigEnterBg)
GATED(hookEnterFg, gOrigEnterFg)

// ---------------------------------------------------------------- entry
__attribute__((constructor)) static void ziruInit() {
  @autoreleasepool {
    initPaths();
    initCrashHandler();
    ZLog(@"ZiruOffline %s loaded, bundle=%@", ZIRU_MOD_VERSION, NSBundle.mainBundle.bundlePath);

    // Initialize original function pointers via dlsym
    orig_open = (int (*)(const char*, int, ...))dlsym(RTLD_DEFAULT, "open");
    orig_access = (int (*)(const char*, int))dlsym(RTLD_DEFAULT, "access");
    orig_stat = (int (*)(const char*, struct stat*))dlsym(RTLD_DEFAULT, "stat");
    orig_lstat = (int (*)(const char*, struct stat*))dlsym(RTLD_DEFAULT, "lstat");
    orig_unlink = (int (*)(const char*))dlsym(RTLD_DEFAULT, "unlink");
    orig_unlinkat = (int (*)(int, const char*, int))dlsym(RTLD_DEFAULT, "unlinkat");
    orig_remove = (int (*)(const char*))dlsym(RTLD_DEFAULT, "remove");

    // Register File I/O hooks via fishhook
    struct z_rebinding rebindings[] = {
      {"open", (void*)hook_open, (void**)&orig_open},
      {"access", (void*)hook_access, (void**)&orig_access},
      {"stat", (void*)hook_stat, (void**)&orig_stat},
      {"lstat", (void*)hook_lstat, (void**)&orig_lstat},
      {"unlink", (void*)hook_unlink, (void**)&orig_unlink},
      {"unlinkat", (void*)hook_unlinkat, (void**)&orig_unlinkat},
      {"remove", (void*)hook_remove, (void**)&orig_remove},
    };
    z_rebind_symbols(rebindings, sizeof(rebindings) / sizeof(rebindings[0]));

    // Swizzle NSFileManager for commentary protection
    Class fmCls = NSFileManager.class;
    gOrigFmRemovePath = swizzleInstance(fmCls, @selector(removeItemAtPath:error:), (IMP)hookFmRemovePath);
    gOrigFmRemoveURL = swizzleInstance(fmCls, @selector(removeItemAtURL:error:), (IMP)hookFmRemoveURL);

    // Network protocol
    [NSURLProtocol registerClass:ZiruURLProtocol.class];
    Class cfg = NSURLSessionConfiguration.class;
    gOrigDefaultCfg = swizzleClassMethod(cfg, @selector(defaultSessionConfiguration), (IMP)hookDefaultCfg);
    gOrigEphemeralCfg = swizzleClassMethod(cfg, @selector(ephemeralSessionConfiguration), (IMP)hookEphemeralCfg);
    SEL sr = NSSelectorFromString(@"setRequest:cookie:isPost:sendData:sendDataSize:useGzip:");
    const char* names[3] = {"HttpImplAsyncRequest", "HttpImplRequest", "HttpImpl2Request"};
    IMP hooks[3] = {(IMP)hookSetRequest<0>, (IMP)hookSetRequest<1>, (IMP)hookSetRequest<2>};
    for (int i = 0; i < 3; i++) {
      Class c = objc_getClass(names[i]);
      if (c) gOrigSetReq[i] = (SetReqFn)swizzleInstance(c, sr, hooks[i]);
      ZLog(@"hook %s: %s", names[i], gOrigSetReq[i] ? "ok" : "MISSING");
    }
    Class d = objc_getClass("IOSAppDelegate");
    if (d) {
      gOrigDidFinish = swizzleInstance(d, @selector(application:didFinishLaunchingWithOptions:), (IMP)hookDidFinish);
      gOrigBecomeActive = swizzleInstance(d, @selector(applicationDidBecomeActive:), (IMP)hookBecomeActive);
      gOrigResignActive = swizzleInstance(d, @selector(applicationWillResignActive:), (IMP)hookResignActive);
      gOrigEnterBg = swizzleInstance(d, @selector(applicationDidEnterBackground:), (IMP)hookEnterBg);
      gOrigEnterFg = swizzleInstance(d, @selector(applicationWillEnterForeground:), (IMP)hookEnterFg);
    } else {
      ZLog(@"IOSAppDelegate not found — launch gating disabled");
      gGameStarted = true;
    }
  }
}
