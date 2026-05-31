#import <InputMethodKit/InputMethodKit.h>
#import <Cocoa/Cocoa.h>

#import "TroutInputController.h"

// Entry point for the Trout input method server. IMKServer reads the connection
// name and controller class from the bundle's Info.plist and instantiates a
// TroutInputController per input session.
int main(int argc, const char* argv[]) {
    @autoreleasepool {
        NSBundle* bundle = [NSBundle mainBundle];
        NSString* connName = bundle.infoDictionary[@"InputMethodConnectionName"];
        if (connName.length == 0) connName = @"Trout_1_Connection";

        IMKServer* server __attribute__((unused)) =
            [[IMKServer alloc] initWithName:connName
                           bundleIdentifier:bundle.bundleIdentifier];

        [[NSApplication sharedApplication] run];
    }
    return 0;
}
