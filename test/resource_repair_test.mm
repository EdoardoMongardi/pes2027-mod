#import <Foundation/Foundation.h>
#import <CommonCrypto/CommonDigest.h>
#include <unistd.h>
static NSString* gDocs;
#define ZLog(...) NSLog(__VA_ARGS__)
#include "../ios/ZiruResources.inc"

static void write(NSString* dir, NSString* name, NSString* text) {
  [NSFileManager.defaultManager createDirectoryAtPath:dir withIntermediateDirectories:YES attributes:nil error:nil];
  NSCAssert([text writeToFile:[dir stringByAppendingPathComponent:name] atomically:YES encoding:NSUTF8StringEncoding error:nil], @"fixture write");
}
static NSDictionary* authorize(NSArray* names, NSString* dir) {
  NSMutableDictionary* result = [NSMutableDictionary dictionary];
  for (NSString* name in names) result[name] = @{@"sha256": resourceSHA256([dir stringByAppendingPathComponent:name])};
  return result;
}
int main() {
  @autoreleasepool {
    NSFileManager* fm = NSFileManager.defaultManager;
    gDocs = [NSTemporaryDirectory() stringByAppendingPathComponent:NSUUID.UUID.UUIDString];
    NSString* source = [gDocs stringByAppendingPathComponent:@"source"];
    NSString* dest = [gDocs stringByAppendingPathComponent:@"pak"];
    NSArray* names = @[@"pc7999_mobile_ios.pak", @"pc7999_mobile_ios.utoc", @"pc7999_mobile_ios.ucas"];
    for (NSString* name in names) write(source, name, [@"bundle-" stringByAppendingString:name]);
    NSDictionary* manifest = authorize(names, source);
    NSCAssert(installResourceGroup(names, source, dest, manifest), @"install complete container");
    NSCAssert(installResourceGroup(names, source, dest, manifest), @"idempotent rerun");
    // Receipt permits updating only a previously installed version.
    for (NSString* name in names) write(source, name, [@"new-bundle-" stringByAppendingString:name]);
    manifest = authorize(names, source);
    NSCAssert(installResourceGroup(names, source, dest, manifest), @"managed container upgrade");
    // A genuine small companion PAK is a mod, not a disposable stub.
    write(dest, names[0], @"custom pak");
    NSString* oldToc = resourceSHA256([dest stringByAppendingPathComponent:names[1]]);
    for (NSString* name in names) write(source, name, @"third version");
    manifest = authorize(names, source);
    NSCAssert(!installResourceGroup(names, source, dest, manifest), @"custom group must survive");
    NSCAssert([oldToc isEqualToString:resourceSHA256([dest stringByAppendingPathComponent:names[1]])], @"no mixed-version triple");
    // An interrupted/incomplete bundle never removes the existing group.
    [fm removeItemAtPath:[source stringByAppendingPathComponent:names[2]] error:nil];
    NSString* emptyDest = [gDocs stringByAppendingPathComponent:@"missing-source-destination"];
    NSCAssert(!installResourceGroup(names, source, emptyDest, manifest), @"missing UCAS rejected");
    NSCAssert(![fm fileExistsAtPath:emptyDest], @"incomplete source never starts installation");
    NSString* cpkDir = [gDocs stringByAppendingPathComponent:@"cpk"];
    NSString* cpk = @"dt870_mobile_and.cpk";
    write(source, cpk, @"known repair"); write(cpkDir, cpk, @"unknown custom database");
    NSCAssert(!installResourceGroup(@[cpk], source, cpkDir, authorize(@[cpk], source)), @"unknown CPK preserved regardless of size");
    NSString* broken = [gDocs stringByAppendingPathComponent:@"broken"];
    [fm createDirectoryAtPath:broken withIntermediateDirectories:YES attributes:nil error:nil];
    [fm createSymbolicLinkAtPath:[broken stringByAppendingPathComponent:cpk] withDestinationPath:@"/missing/user-mod.cpk" error:nil];
    NSCAssert(!installResourceGroup(@[cpk], source, broken, authorize(@[cpk], source)), @"unknown broken link preserved");
    [fm removeItemAtPath:gDocs error:nil];
    NSLog(@"Resource preservation scenarios passed");
  }
}
