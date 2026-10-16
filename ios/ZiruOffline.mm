// ZiruOffline.dylib — loaded into eFootball iOS 11.1.1 (PESmobile).
//  * Runs the C++ port of the Ziru offline server in-process.
//  * Redirects the game's HttpImpl* requests to https://127.0.0.1:8085 (same as Ziru's Android HttpImpl patch)
//    and answers them through an NSURLProtocol — no sockets, no TLS.
//  * Gates the UE4 launch until the separate data pack (pak/cpk/livedata) has been imported.
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <objc/runtime.h>
#include <sys/stat.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>

#include "../core/datapack.hpp"
#include "../core/server.hpp"

#define ZIRU_MOD_VERSION "1.0.0"

// ---------------------------------------------------------------- paths & log
static NSString* gDocs;
static NSString* gSupport;   // Library/Application Support/PES2027
static NSString* gLogPath;
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
  NSDictionary* a = [fm attributesOfItemAtPath:gLogPath error:nil];
  if (a && [a fileSize] > (8u << 20)) {
    NSString* old = [logDir stringByAppendingPathComponent:@"ziru.old.log"];
    [fm removeItemAtPath:old error:nil];
    [fm moveItemAtPath:gLogPath toPath:old error:nil];
  }
  gBundleAssets = [NSBundle.mainBundle.bundlePath stringByAppendingPathComponent:@"ziru"];
  zs::setLogSink(logSink);
}

// ---------------------------------------------------------------- server
static std::unique_ptr<zsv::OfflineServer> gServer;
static std::once_flag gServerOnce;

static zsv::OfflineServer& server() {
  std::call_once(gServerOnce, [] {
    zsv::Config cfg;
    cfg.assetsDir = gBundleAssets.fileSystemRepresentation;
    std::string id = zd::activeId(layout());
    if (!id.empty()) cfg.livedataDir = zd::dataDir(layout(), id) + "/livedata";
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
+ (BOOL)canInitWithRequest:(NSURLRequest*)request { return shouldHandleURL(request.URL); }
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
  // make sure we patch this class and not a superclass implementation
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
  if (red != orig) ZLog(@"[REDIRECT] %s -> %s", orig.c_str(), red.c_str());
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

static BOOL dataReady(NSString** why) {
  std::string err;
  zd::Layout L = layout();
  std::string id = zd::activeId(L);
  bool ok = zd::quickVerify(L, id, err);
  if (!ok && why) *why = [NSString stringWithUTF8String:err.c_str()];
  return ok;
}

@interface ZiruImportVC : UIViewController <UIDocumentPickerDelegate>
@property(nonatomic, strong) UILabel* status;
@property(nonatomic, strong) UILabel* detail;
@property(nonatomic, strong) UIProgressView* bar;
@property(nonatomic, strong) UIButton* pick;
@property(nonatomic, strong) UIButton* local;
@property(nonatomic, copy) NSString* localZip;
@property(atomic) BOOL busy;
@end

@implementation ZiruImportVC
- (UIInterfaceOrientationMask)supportedInterfaceOrientations { return UIInterfaceOrientationMaskLandscape; }
- (BOOL)prefersStatusBarHidden { return YES; }

static UIButton* makeButton(NSString* title) {
  UIButton* b = [UIButton buttonWithType:UIButtonTypeSystem];
  [b setTitle:title forState:UIControlStateNormal];
  b.titleLabel.font = [UIFont boldSystemFontOfSize:18];
  b.backgroundColor = [UIColor colorWithRed:0.10 green:0.45 blue:0.95 alpha:1];
  [b setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
  b.layer.cornerRadius = 10;
  b.contentEdgeInsets = UIEdgeInsetsMake(12, 20, 12, 20);
  b.translatesAutoresizingMaskIntoConstraints = NO;
  return b;
}

- (void)viewDidLoad {
  [super viewDidLoad];
  self.view.backgroundColor = [UIColor colorWithRed:0.05 green:0.07 blue:0.11 alpha:1];
  UILabel* title = [UILabel new];
  title.text = @"PES 2027 Offline — Nhập dữ liệu game";
  title.font = [UIFont boldSystemFontOfSize:24];
  title.textColor = UIColor.whiteColor;
  self.status = [UILabel new];
  self.status.textColor = [UIColor colorWithWhite:0.9 alpha:1];
  self.status.font = [UIFont systemFontOfSize:17];
  self.status.numberOfLines = 0;
  self.status.textAlignment = NSTextAlignmentCenter;
  self.detail = [UILabel new];
  self.detail.textColor = [UIColor colorWithWhite:0.6 alpha:1];
  self.detail.font = [UIFont systemFontOfSize:13];
  self.detail.numberOfLines = 0;
  self.detail.textAlignment = NSTextAlignmentCenter;
  self.bar = [[UIProgressView alloc] initWithProgressViewStyle:UIProgressViewStyleDefault];
  self.bar.hidden = YES;
  self.pick = makeButton(@"Chọn gói data (.zip) từ Tệp");
  [self.pick addTarget:self action:@selector(onPick) forControlEvents:UIControlEventTouchUpInside];
  self.local = makeButton(@"Nhập gói trong thư mục ứng dụng");
  [self.local addTarget:self action:@selector(onLocal) forControlEvents:UIControlEventTouchUpInside];
  UIStackView* st = [[UIStackView alloc] initWithArrangedSubviews:@[ title, self.status, self.bar, self.pick, self.local, self.detail ]];
  st.axis = UILayoutConstraintAxisVertical;
  st.alignment = UIStackViewAlignmentCenter;
  st.spacing = 16;
  st.translatesAutoresizingMaskIntoConstraints = NO;
  [self.view addSubview:st];
  [NSLayoutConstraint activateConstraints:@[
    [st.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
    [st.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor],
    [st.widthAnchor constraintLessThanOrEqualToAnchor:self.view.widthAnchor multiplier:0.85],
    [self.bar.widthAnchor constraintEqualToConstant:420],
  ]];
  [self refresh];
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
  dataReady(&why);
  self.localZip = [self findLocalZip];
  self.local.hidden = self.localZip == nil;
  if (self.localZip) [self.local setTitle:[NSString stringWithFormat:@"Nhập %@", self.localZip.lastPathComponent] forState:UIControlStateNormal];
  self.status.text = @"Chưa có dữ liệu game. Hãy chọn file PES2027-data-….zip (khoảng 3 GB).\n"
                     @"Có thể chép file vào thư mục \"Trên iPhone › eFootball\" bằng app Tệp hoặc Finder/iTunes.";
  self.detail.text = [NSString stringWithFormat:@"Bản mod %s • %@", ZIRU_MOD_VERSION, why ?: @""];
}

- (void)onPick {
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
                                                                   message:@"Dữ liệu đã nhập. Có thể xóa file .zip gốc để giải phóng dung lượng."
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
    ZLog(@"ZiruOffline %s loaded, bundle=%@", ZIRU_MOD_VERSION, NSBundle.mainBundle.bundlePath);
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
