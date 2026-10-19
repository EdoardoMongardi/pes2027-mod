#import <UIKit/UIKit.h>
#import <WebKit/WebKit.h>

static void EditorLog(NSString* message) {
  NSURL* docs = [[NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] firstObject];
  NSURL* folder = [docs URLByAppendingPathComponent:@"Diagnostics" isDirectory:YES];
  [NSFileManager.defaultManager createDirectoryAtURL:folder withIntermediateDirectories:YES attributes:nil error:nil];
  NSURL* file = [folder URLByAppendingPathComponent:@"editor.log"];
  NSDictionary* attr = [NSFileManager.defaultManager attributesOfItemAtPath:file.path error:nil];
  if ([attr fileSize] > 1024*1024) {
    NSURL* previous = [folder URLByAppendingPathComponent:@"editor.previous.log"];
    [NSFileManager.defaultManager removeItemAtURL:previous error:nil];
    [NSFileManager.defaultManager moveItemAtURL:file toURL:previous error:nil];
  }
  NSData* data = [[NSString stringWithFormat:@"%@ %@\n", NSDate.date, message] dataUsingEncoding:NSUTF8StringEncoding];
  if (![NSFileManager.defaultManager fileExistsAtPath:file.path]) [data writeToURL:file atomically:YES];
  else { NSFileHandle* f = [NSFileHandle fileHandleForWritingAtPath:file.path]; [f seekToEndOfFile]; [f writeData:data]; [f closeFile]; }
}

@interface EditorViewController : UIViewController <WKScriptMessageHandler, WKNavigationDelegate, WKUIDelegate, UIDocumentPickerDelegate>
@property(nonatomic,strong) WKWebView* web;
@end
@implementation EditorViewController
- (void)viewDidLoad {
  [super viewDidLoad]; self.view.backgroundColor = UIColor.systemBackgroundColor;
  WKWebViewConfiguration* config = [WKWebViewConfiguration new];
  [config.userContentController addScriptMessageHandler:self name:@"efootbase"];
  self.web = [[WKWebView alloc] initWithFrame:CGRectZero configuration:config];
  self.web.navigationDelegate = self; self.web.UIDelegate = self;
  self.web.translatesAutoresizingMaskIntoConstraints = NO; [self.view addSubview:self.web];
  [NSLayoutConstraint activateConstraints:@[
    [self.web.topAnchor constraintEqualToAnchor:self.view.topAnchor],
    [self.web.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor],
    [self.web.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
    [self.web.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor]]];
  NSURL* folder = [NSBundle.mainBundle.resourceURL URLByAppendingPathComponent:@"web" isDirectory:YES];
  [self.web loadFileURL:[folder URLByAppendingPathComponent:@"index.html"] allowingReadAccessToURL:folder];
  EditorLog(@"launch editor; local web UI and native export bridge");
}
- (void)notify:(NSString*)name payload:(NSDictionary*)payload {
  NSData* data = [NSJSONSerialization dataWithJSONObject:payload options:0 error:nil];
  NSString* json = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
  NSString* js = [NSString stringWithFormat:@"window.dispatchEvent(new CustomEvent('%@',{detail:%@}));", name, json ?: @"{}"];
  [self.web evaluateJavaScript:js completionHandler:nil];
}
- (void)userContentController:(WKUserContentController*)controller didReceiveScriptMessage:(WKScriptMessage*)message {
  if (![message.body isKindOfClass:NSDictionary.class]) return;
  NSDictionary* body = message.body; NSString* action = body[@"action"];
  if ([action isEqual:@"log"]) {
    NSString* text = [body[@"message"] isKindOfClass:NSString.class] ? body[@"message"] : @"web error";
    EditorLog([@"web: " stringByAppendingString:[text substringToIndex:MIN(text.length,1500)]]); return;
  }
  if (![action isEqual:@"export"] || ![body[@"base64"] isKindOfClass:NSString.class] || ![body[@"name"] isKindOfClass:NSString.class]) return;
  NSString* name = body[@"name"]; NSString* encoded = body[@"base64"];
  NSCharacterSet* forbidden = [[NSCharacterSet characterSetWithCharactersInString:@"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-"] invertedSet];
  if (name.length > 100 || [name rangeOfCharacterFromSet:forbidden].location != NSNotFound ||
      ![@[@"zip",@"json"] containsObject:name.pathExtension.lowercaseString] || encoded.length > 90*1024*1024) {
    [self notify:@"efootbase-export-error" payload:@{@"message":@"Tên hoặc kích thước file không hợp lệ."}]; return;
  }
  NSData* bytes = [[NSData alloc] initWithBase64EncodedString:encoded options:0];
  if (!bytes || bytes.length > 64*1024*1024) { [self notify:@"efootbase-export-error" payload:@{@"message":@"Không đọc được nội dung xuất."}]; return; }
  NSURL* docs = [[NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] firstObject];
  NSURL* folder = [[docs URLByAppendingPathComponent:@"Exports" isDirectory:YES] URLByAppendingPathComponent:NSUUID.UUID.UUIDString isDirectory:YES];
  NSError* error = nil;
  [NSFileManager.defaultManager createDirectoryAtURL:folder withIntermediateDirectories:YES attributes:nil error:&error];
  NSURL* file = [folder URLByAppendingPathComponent:name];
  if (error || ![bytes writeToURL:file options:NSDataWritingAtomic error:&error]) {
    EditorLog([@"export failed: " stringByAppendingString:error.localizedDescription ?: @"write error"]);
    [self notify:@"efootbase-export-error" payload:@{@"message":@"Không lưu được file xuất."}]; return;
  }
  EditorLog([NSString stringWithFormat:@"export %@ bytes=%lu", name, (unsigned long)bytes.length]);
  [self notify:@"efootbase-export-ready" payload:@{@"name":name,@"bytes":@(bytes.length)}];
  UIDocumentPickerViewController* picker = [[UIDocumentPickerViewController alloc] initForExportingURLs:@[file] asCopy:YES];
  picker.delegate = self; [self presentViewController:picker animated:YES completion:nil];
}
- (void)documentPicker:(UIDocumentPickerViewController*)controller didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls {
  EditorLog(@"export saved through Files picker"); [self notify:@"efootbase-export-saved" payload:@{@"ok":@YES}];
}
- (void)documentPickerWasCancelled:(UIDocumentPickerViewController*)controller {
  EditorLog(@"Files picker cancelled; export remains in app Documents/Exports");
}
- (void)webView:(WKWebView*)webView didFailNavigation:(WKNavigation*)navigation withError:(NSError*)error { EditorLog(error.localizedDescription); }
- (void)webView:(WKWebView*)webView runJavaScriptAlertPanelWithMessage:(NSString*)message initiatedByFrame:(WKFrameInfo*)frame completionHandler:(void (^)(void))completion {
  UIAlertController* alert = [UIAlertController alertControllerWithTitle:@"eFootbase" message:message preferredStyle:UIAlertControllerStyleAlert];
  [alert addAction:[UIAlertAction actionWithTitle:@"Đóng" style:UIAlertActionStyleDefault handler:^(UIAlertAction* a){completion();}]];
  [self presentViewController:alert animated:YES completion:nil];
}
- (void)webView:(WKWebView*)webView runJavaScriptConfirmPanelWithMessage:(NSString*)message initiatedByFrame:(WKFrameInfo*)frame completionHandler:(void (^)(BOOL))completion {
  UIAlertController* alert = [UIAlertController alertControllerWithTitle:@"eFootbase" message:message preferredStyle:UIAlertControllerStyleAlert];
  [alert addAction:[UIAlertAction actionWithTitle:@"Hủy" style:UIAlertActionStyleCancel handler:^(UIAlertAction* a){completion(NO);}]];
  [alert addAction:[UIAlertAction actionWithTitle:@"Tiếp tục" style:UIAlertActionStyleDefault handler:^(UIAlertAction* a){completion(YES);}]];
  [self presentViewController:alert animated:YES completion:nil];
}
@end
@interface EditorAppDelegate : UIResponder <UIApplicationDelegate>
@property(nonatomic,strong) UIWindow* window;
@end
@implementation EditorAppDelegate
- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)options {
  self.window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
  self.window.rootViewController = [EditorViewController new]; [self.window makeKeyAndVisible]; return YES;
}
@end
int main(int argc, char* argv[]) { @autoreleasepool { return UIApplicationMain(argc,argv,nil,NSStringFromClass(EditorAppDelegate.class)); } }
