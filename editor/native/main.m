#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#import "GameAppearance.h"
@interface StudioDelegate : NSObject <NSApplicationDelegate,NSWindowDelegate,WKScriptMessageHandler,WKNavigationDelegate,WKUIDelegate>
@property NSWindow *window;
@property WKWebView *web;
@property NSURL *resources;
@property NSURL *recovery;
@property NSMutableArray<NSTask *> *tasks;
@property BOOL hasUnsavedChanges;
@property NSString *pendingPath;
@end
@implementation StudioDelegate
- (void)applicationDidFinishLaunching:(NSNotification *)notification {
 self.resources=NSBundle.mainBundle.resourceURL;
 NSURL *support=[NSFileManager.defaultManager URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask].firstObject;
 self.recovery=[[support URLByAppendingPathComponent:@"LotA Studio"] URLByAppendingPathComponent:@"recovery.lota"];
 [NSFileManager.defaultManager createDirectoryAtURL:self.recovery.URLByDeletingLastPathComponent withIntermediateDirectories:YES attributes:nil error:nil];
 self.tasks=[NSMutableArray new];
 NSMenu *menu=[NSMenu new],*appMenu=[NSMenu new];NSMenuItem *appItem=[NSMenuItem new];[menu addItem:appItem];[appItem setSubmenu:appMenu];[appMenu addItemWithTitle:@"About EveNGINE Studio" action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];[appMenu addItem:NSMenuItem.separatorItem];[appMenu addItemWithTitle:@"Quit EveNGINE Studio" action:@selector(terminate:) keyEquivalent:@"q"];
 NSMenuItem *editItem=[NSMenuItem new];[menu addItem:editItem];NSMenu *edit=[NSMenu new];[editItem setTitle:@"Edit"];[editItem setSubmenu:edit];[edit addItemWithTitle:@"Copy" action:@selector(copy:) keyEquivalent:@"c"];[edit addItemWithTitle:@"Paste" action:@selector(paste:) keyEquivalent:@"v"];[edit addItemWithTitle:@"Select All" action:@selector(selectAll:) keyEquivalent:@"a"];
 NSApp.mainMenu=menu;
 WKWebViewConfiguration *config=[WKWebViewConfiguration new];[config.userContentController addScriptMessageHandler:self name:@"studio"];
 self.web=[[WKWebView alloc] initWithFrame:NSMakeRect(0,0,1360,900) configuration:config];self.web.navigationDelegate=self;self.web.UIDelegate=self;
 self.window=[[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,1360,900) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable|NSWindowStyleMaskResizable backing:NSBackingStoreBuffered defer:NO];self.window.title=@"EveNGINE Studio";self.window.minSize=NSMakeSize(1080,700);self.window.contentView=self.web;self.window.delegate=self;[self.window center];[self.window makeKeyAndOrderFront:nil];
 NSURL *ui=[self.resources URLByAppendingPathComponent:@"ui"];[self.web loadFileURL:[ui URLByAppendingPathComponent:@"index.html"] allowingReadAccessToURL:ui];[NSApp activateIgnoringOtherApps:YES];
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender{return YES;}
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
 if(!self.hasUnsavedChanges)return NSTerminateNow;
 NSAlert *a=[NSAlert new];a.messageText=@"Close EveNGINE Studio?";a.informativeText=@"Use Save to keep a portable project. Your latest edits are also kept in local recovery.";[a addButtonWithTitle:@"Close"];[a addButtonWithTitle:@"Keep editing"];return [a runModal]==NSAlertFirstButtonReturn?NSTerminateNow:NSTerminateCancel;
}
- (void)applicationWillTerminate:(NSNotification *)notification {for(NSTask *task in self.tasks)if(task.running)[task terminate];}
- (void)reply:(id)request data:(NSDictionary *)result {
 NSData *data=[NSJSONSerialization dataWithJSONObject:@[request,result] options:0 error:nil];NSString *json=[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];NSString *script=[NSString stringWithFormat:@"window.studioReply(...%@)",json];[self.web evaluateJavaScript:script completionHandler:nil];
}
- (NSData *)projectData:(NSDictionary *)project error:(NSError **)error {
 if(![project isKindOfClass:NSDictionary.class]||![project[@"format"] isEqual:@"lota-studio"]){if(error)*error=[NSError errorWithDomain:@"Studio" code:1 userInfo:@{NSLocalizedDescriptionKey:@"Invalid project"}];return nil;}return [NSJSONSerialization dataWithJSONObject:project options:NSJSONWritingPrettyPrinted error:error];
}
- (BOOL)write:(NSData *)data url:(NSURL *)url error:(NSError **)error {return [data writeToURL:url options:NSDataWritingAtomic error:error];}
- (NSURL *)temporaryProject:(NSDictionary *)project error:(NSError **)error {
 NSURL *folder=[[NSURL fileURLWithPath:NSTemporaryDirectory()] URLByAppendingPathComponent:NSUUID.UUID.UUIDString];if(![NSFileManager.defaultManager createDirectoryAtURL:folder withIntermediateDirectories:YES attributes:nil error:error])return nil;NSURL *path=[folder URLByAppendingPathComponent:@"project.lota"];NSData *data=[self projectData:project error:error];if(!data||![self write:data url:path error:error])return nil;NSURL *font=[self.resources URLByAppendingPathComponent:@"font.pam"];if(![NSFileManager.defaultManager copyItemAtURL:font toURL:[folder URLByAppendingPathComponent:@"font.pam"] error:error])return nil;return path;
}
- (BOOL)validate:(NSURL *)project error:(NSError **)error {
 NSTask *task=[NSTask new];task.executableURL=[self.resources URLByAppendingPathComponent:@"Runtime"];task.arguments=@[@"--project",project.path,@"--validate"];NSPipe *pipe=[NSPipe pipe];task.standardError=pipe;task.standardOutput=[NSPipe pipe];if(![task launchAndReturnError:error])return NO;[task waitUntilExit];NSData *data=[pipe.fileHandleForReading readDataToEndOfFile];if(task.terminationStatus!=0){NSString *message=[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];if(error)*error=[NSError errorWithDomain:@"Studio" code:2 userInfo:@{NSLocalizedDescriptionKey:message?:@"Runtime rejected project"}];return NO;}return YES;
}
- (void)userContentController:(WKUserContentController *)controller didReceiveScriptMessage:(WKScriptMessage *)message {
 NSDictionary *body=message.body;id request=body[@"id"];NSString *action=body[@"action"];if(![body isKindOfClass:NSDictionary.class]||!request||![action isKindOfClass:NSString.class])return;
 NSError *error=nil;
 if([action isEqual:@"open"]){NSOpenPanel *panel=[NSOpenPanel openPanel];panel.allowedFileTypes=@[@"eve",@"eve-object",@"lota",@"lota-object"];panel.canChooseDirectories=NO;panel.allowsMultipleSelection=NO;if([panel runModal]!=NSModalResponseOK){[self reply:request data:@{@"cancelled":@YES}];return;}NSData *data=[NSData dataWithContentsOfURL:panel.URL options:0 error:&error];NSDictionary *project=data?[NSJSONSerialization JSONObjectWithData:data options:0 error:&error]:nil;if(project){self.hasUnsavedChanges=NO;[self reply:request data:@{@"project":project,@"path":panel.URL.path}];return;}}
 else if([action isEqual:@"save"]||[action isEqual:@"autosave"]){NSURL *url=nil;if([action isEqual:@"autosave"]){url=self.recovery;self.hasUnsavedChanges=YES;}else if([body[@"path"] isKindOfClass:NSString.class]&&[body[@"path"] length])url=[NSURL fileURLWithPath:body[@"path"]];else{NSSavePanel *panel=[NSSavePanel savePanel];panel.allowedFileTypes=@[@"eve",@"lota"];panel.nameFieldStringValue=@"My Adventure.eve";if([panel runModal]!=NSModalResponseOK){[self reply:request data:@{@"cancelled":@YES}];return;}url=panel.URL;}NSData *data=[self projectData:body[@"project"] error:&error];if(data&&[self write:data url:url error:&error]){if([action isEqual:@"save"])self.hasUnsavedChanges=NO;[self reply:request data:@{@"path":url.path}];return;}}
 else if([action isEqual:@"recover"]){NSData *data=[NSData dataWithContentsOfURL:self.recovery options:0 error:&error];NSDictionary *project=data?[NSJSONSerialization JSONObjectWithData:data options:0 error:&error]:nil;if(project){self.hasUnsavedChanges=YES;[self reply:request data:@{@"project":project}];return;}}
 else if([action isEqual:@"template"]){NSSavePanel *panel=[NSSavePanel savePanel];panel.allowedFileTypes=@[@"eve-object",@"lota-object"];panel.nameFieldStringValue=body[@"name"];if([panel runModal]!=NSModalResponseOK){[self reply:request data:@{@"cancelled":@YES}];return;}NSData *data=[NSJSONSerialization dataWithJSONObject:body[@"template"] options:NSJSONWritingPrettyPrinted error:&error];if(data&&[self write:data url:panel.URL error:&error]){[self reply:request data:@{@"message":@"Object template saved."}];return;}}
 else if([action isEqual:@"png"]){NSSavePanel *panel=[NSSavePanel savePanel];panel.allowedFileTypes=@[@"png"];panel.nameFieldStringValue=body[@"name"];if([panel runModal]!=NSModalResponseOK){[self reply:request data:@{@"cancelled":@YES}];return;}NSData *data=[[NSData alloc] initWithBase64EncodedString:body[@"data"] options:0];if(data&&[self write:data url:panel.URL error:&error]){[self reply:request data:@{@"message":@"PNG saved."}];return;}}
 else if([action isEqual:@"play"]||[action isEqual:@"export"]){NSURL *project=[self temporaryProject:body[@"project"] error:&error];if(project&&[self validate:project error:&error]){
 if([action isEqual:@"play"]){NSTask *task=[NSTask new];task.executableURL=[self.resources URLByAppendingPathComponent:@"Runtime"];task.arguments=@[@"--project",project.path,@"--scene",body[@"scene"]?:body[@"project"][@"start"]];if([task launchAndReturnError:&error]){[self.tasks addObject:task];[self reply:request data:@{@"message":@"Native playtest launched. F3 shows collision; Esc opens Pause."}];return;}}
 else {NSSavePanel *panel=[NSSavePanel savePanel];panel.allowedFileTypes=@[@"app"];panel.nameFieldStringValue=@"My Adventure.app";if([panel runModal]!=NSModalResponseOK){[self reply:request data:@{@"cancelled":@YES}];return;}
 NSURL *destination=panel.URL;BOOL ok=EveExportGame(body[@"project"],[self.resources URLByAppendingPathComponent:@"Runtime"],[self.resources URLByAppendingPathComponent:@"font.pam"],[[self.resources.URLByDeletingLastPathComponent URLByAppendingPathComponent:@"Frameworks"] URLByAppendingPathComponent:@"SDL3.framework"],[self.resources URLByAppendingPathComponent:@"Studio.icns"],destination,&error);
 if(ok){[self reply:request data:@{@"message":[@"Exported " stringByAppendingString:destination.lastPathComponent]}];[NSWorkspace.sharedWorkspace activateFileViewerSelectingURLs:@[destination]];return;}

 }}}
 else error=[NSError errorWithDomain:@"Studio" code:3 userInfo:@{NSLocalizedDescriptionKey:@"Unknown editor action"}];
 [self reply:request data:@{@"error":error.localizedDescription?:@"Operation failed"}];
}
- (BOOL)windowShouldClose:(NSWindow *)sender {if(!self.hasUnsavedChanges)return YES;if([self applicationShouldTerminate:NSApp]==NSTerminateNow){self.hasUnsavedChanges=NO;return YES;}return NO;}
- (void)webView:(WKWebView *)webView runOpenPanelWithParameters:(WKOpenPanelParameters *)parameters initiatedByFrame:(WKFrameInfo *)frame completionHandler:(void (^)(NSArray<NSURL *> *))completionHandler {NSOpenPanel *panel=[NSOpenPanel openPanel];panel.allowsMultipleSelection=parameters.allowsMultipleSelection;panel.canChooseDirectories=parameters.allowsDirectories;completionHandler([panel runModal]==NSModalResponseOK?panel.URLs:nil);}
- (BOOL)application:(NSApplication *)sender openFile:(NSString *)filename {self.pendingPath=filename;if(!self.web.loading)[self loadPending];return YES;}
- (void)loadPending {if(!self.pendingPath||!self.web)return;NSData *data=[NSData dataWithContentsOfFile:self.pendingPath];id project=data?[NSJSONSerialization JSONObjectWithData:data options:0 error:nil]:nil;if(project){NSData *json=[NSJSONSerialization dataWithJSONObject:@[project,self.pendingPath] options:0 error:nil];NSString *script=[NSString stringWithFormat:@"window.studioLoad(...%@)",[[NSString alloc] initWithData:json encoding:NSUTF8StringEncoding]];[self.web evaluateJavaScript:script completionHandler:nil];}self.pendingPath=nil;}
- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation {[self loadPending];}
- (void)webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler {NSURL *url=navigationAction.request.URL;BOOL local=[url.scheme isEqual:@"file"]&&[url.path hasPrefix:[self.resources URLByAppendingPathComponent:@"ui"].path];decisionHandler(local?WKNavigationActionPolicyAllow:WKNavigationActionPolicyCancel);}
@end
static StudioDelegate *studioApplicationDelegate;
int main(int argc,const char **argv){@autoreleasepool{NSApplication *app=NSApplication.sharedApplication;studioApplicationDelegate=[StudioDelegate new];app.delegate=studioApplicationDelegate;[app setActivationPolicy:NSApplicationActivationPolicyRegular];[app run];}return 0;}
