#import <AppKit/AppKit.h>
#import "AppController.h"

@interface TroutAppDelegate : NSObject <NSApplicationDelegate>
@property(nonatomic, strong) AppController* controller;
@end

@implementation TroutAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)note {
    self.controller = [[AppController alloc] init];
    [self.controller startServices];
}

- (void)applicationWillTerminate:(NSNotification*)note {
    [self.controller shutdown];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    return NO; // menu-bar agent: keep running with no windows
}

@end

int main(int argc, const char* argv[]) {
    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        // Accessory: no Dock icon, no main menu (LSUIElement also set in plist).
        [app setActivationPolicy:NSApplicationActivationPolicyAccessory];
        TroutAppDelegate* delegate = [[TroutAppDelegate alloc] init];
        app.delegate = delegate;
        [app run];
    }
    return 0;
}
