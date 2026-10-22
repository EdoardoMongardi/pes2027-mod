#import <Foundation/Foundation.h>
#include <cstring>
#define ZLog(...) NSLog(__VA_ARGS__)
#include "../ios/ZiruDisplayRepair.inc"

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
    [fm removeItemAtPath:root error:nil];
  }
}
