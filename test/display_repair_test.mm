#import <Foundation/Foundation.h>
#include <cstring>
#define ZLog(...) NSLog(__VA_ARGS__)
#include "../ios/ZiruDisplayRepair.inc"
#include "../ios/ZiruNativeBase.inc"

int main() {
  @autoreleasepool {
    NSCAssert(isTemporaryDownloadPath("/private/var/mobile/app/tmp/TempData/Download/hash"), @"native temporary image path stays intact");
    NSCAssert(isTemporaryDownloadPath("/app/Library/Caches/Download/hash"), @"Foundation caches stay intact");
    NSCAssert(!isTemporaryDownloadPath("/app/Documents/Download/pc9257_mobile_ios.utoc"), @"live update can be redirected");
    NSString* ini = @"[SystemSettings]\nr.MobileHDR=1\nr.Color.Saturation=1.3\nr.TonemapperGamma=2.2\nr.ShadowQuality=2\n[Other]\nName=User\n";
    NSString* clean = cleanLegacyGraphics(ini,NO);
    NSCAssert([clean isEqualToString:@"[SystemSettings]\nr.ShadowQuality=2\n[Other]\nName=User\n"], @"only legacy color overrides removed");
    NSCAssert([cleanLegacyGraphics(@"-t.MaxFPS=120 -r.MobileHDR=1 -log",YES) isEqualToString:@"-t.MaxFPS=120  -log"], @"keep unrelated switches");
    NSString* root = [NSTemporaryDirectory() stringByAppendingPathComponent:NSUUID.UUID.UUIDString];
    NSString* docs = [root stringByAppendingPathComponent:@"Documents"];
    NSString* config = [root stringByAppendingPathComponent:@"Library/Preferences/Unreal Engine/PesMobile/IOS"];
    NSFileManager* fm=NSFileManager.defaultManager;
    [fm createDirectoryAtPath:config withIntermediateDirectories:YES attributes:nil error:nil];
    [fm createDirectoryAtPath:docs withIntermediateDirectories:YES attributes:nil error:nil];
    NSString* file=[config stringByAppendingPathComponent:@"Engine.ini"];
    [ini writeToFile:file atomically:YES encoding:NSUTF8StringEncoding error:nil];
    repairSavedGraphics(docs,docs);
    NSCAssert([[NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil] isEqualToString:clean], @"saved config repaired");
    repairSavedGraphics(docs,docs);
    NSString* backup=[docs stringByAppendingPathComponent:@"graphics-before-Documents-1-Engine.ini"];
    NSCAssert([[NSString stringWithContentsOfFile:backup encoding:NSUTF8StringEncoding error:nil] isEqualToString:ini], @"original backed up, repeat is idempotent");
    NSString* dataRoot=[root stringByAppendingPathComponent:@"graphics"];
    [fm createDirectoryAtPath:[dataRoot stringByAppendingPathComponent:@"pak"] withIntermediateDirectories:YES attributes:nil error:nil];
    [fm createDirectoryAtPath:[dataRoot stringByAppendingPathComponent:@"pak.native-staging-1"] withIntermediateDirectories:YES attributes:nil error:nil];
    [@"android" writeToFile:[dataRoot stringByAppendingPathComponent:@"pak/old"] atomically:YES encoding:NSUTF8StringEncoding error:nil];
    [@"ios" writeToFile:[dataRoot stringByAppendingPathComponent:@"pak.native-staging-1/native"] atomically:YES encoding:NSUTF8StringEncoding error:nil];
    [@"old marker" writeToFile:[dataRoot stringByAppendingPathComponent:@"china-data-installed.json"] atomically:YES encoding:NSUTF8StringEncoding error:nil];
    NSString* hash=@"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    NSDictionary* next=@{@"native_render_overlay":@{@"manifest_sha256":hash}};
    [[NSJSONSerialization dataWithJSONObject:next options:0 error:nil] writeToFile:[dataRoot stringByAppendingPathComponent:@"china-data-installed.native-staging-1.json"] atomically:YES];
    NSDictionary* pending=@{@"pak_staging":@"pak.native-staging-1",@"pak_backup":@"pak.before-native-1",
      @"marker_staging":@"china-data-installed.native-staging-1.json",@"marker_backup":@"china-data-installed.before-native-1.json",@"manifest_sha256":hash};
    NSString* pendingPath=[dataRoot stringByAppendingPathComponent:@"native-render-pending.json"];
    [[NSJSONSerialization dataWithJSONObject:pending options:0 error:nil] writeToFile:pendingPath atomically:YES];
    NSCAssert(activatePendingNativeBase(dataRoot,docs), @"native swap succeeds before engine launch");
    NSCAssert([fm fileExistsAtPath:[dataRoot stringByAppendingPathComponent:@"pak/native"]], @"native data activated");
    NSCAssert([fm fileExistsAtPath:[dataRoot stringByAppendingPathComponent:@"pak.before-native-1/old"]], @"old graphics preserved");
    NSCAssert(!activatePendingNativeBase(dataRoot,docs), @"activation is idempotent");
    NSMutableDictionary* unsafe=[pending mutableCopy]; unsafe[@"pak_staging"]=@"../outside";
    [[NSJSONSerialization dataWithJSONObject:unsafe options:0 error:nil] writeToFile:pendingPath atomically:YES];
    NSCAssert(!activatePendingNativeBase(dataRoot,docs), @"unsafe descriptor cannot move external paths");
    NSString* rollback=[root stringByAppendingPathComponent:@"rollback"];
    [fm copyItemAtPath:dataRoot toPath:rollback error:nil];
    [fm moveItemAtPath:[rollback stringByAppendingPathComponent:@"pak"] toPath:[rollback stringByAppendingPathComponent:@"pak.native-staging-1"] error:nil];
    [fm moveItemAtPath:[rollback stringByAppendingPathComponent:@"pak.before-native-1"] toPath:[rollback stringByAppendingPathComponent:@"pak"] error:nil];
    [[NSJSONSerialization dataWithJSONObject:next options:0 error:nil] writeToFile:[rollback stringByAppendingPathComponent:@"china-data-installed.native-staging-1.json"] atomically:YES];
    [@"old marker" writeToFile:[rollback stringByAppendingPathComponent:@"china-data-installed.json"] atomically:YES encoding:NSUTF8StringEncoding error:nil];
    [[NSJSONSerialization dataWithJSONObject:pending options:0 error:nil] writeToFile:[rollback stringByAppendingPathComponent:@"native-render-pending.json"] atomically:YES];
    // The copied .active marker intentionally blocks marker publication.
    NSCAssert(!activatePendingNativeBase(rollback,docs), @"failed publication rolls graphics back");
    NSCAssert([fm fileExistsAtPath:[rollback stringByAppendingPathComponent:@"pak/old"]], @"old graphics restored after failure");
    NSCAssert([fm fileExistsAtPath:[rollback stringByAppendingPathComponent:@"pak.native-staging-1/native"]], @"staged graphics retained for retry");
    NSString* holding=[rollback stringByAppendingPathComponent:@"china-data-installed.before-native-1.json.active"];
    [fm moveItemAtPath:holding toPath:[holding stringByAppendingString:@".preserved"] error:nil];
    [fm moveItemAtPath:[rollback stringByAppendingPathComponent:@"pak"] toPath:[rollback stringByAppendingPathComponent:@"pak.before-native-1"] error:nil];
    NSCAssert(activatePendingNativeBase(rollback,docs), @"resume a launch interrupted after backing up old graphics");
    NSCAssert([fm fileExistsAtPath:[rollback stringByAppendingPathComponent:@"pak/native"]], @"resumed activation uses native graphics");
    [fm removeItemAtPath:root error:nil];
  }
}
