#include "Permissions.h"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <CoreGraphics/CoreGraphics.h>

namespace trout {

bool Permissions::accessibilityTrusted(bool prompt) {
    if (prompt) {
        NSDictionary* opts = @{ (__bridge id)kAXTrustedCheckOptionPrompt: @YES };
        return AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)opts);
    }
    return AXIsProcessTrusted();
}

bool Permissions::screenRecordingGranted() {
    return CGPreflightScreenCaptureAccess();
}

void Permissions::requestScreenRecording() {
    CGRequestScreenCaptureAccess();
}

void Permissions::openAccessibilitySettings() {
    NSURL* url = [NSURL URLWithString:
        @"x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility"];
    [[NSWorkspace sharedWorkspace] openURL:url];
}

void Permissions::openScreenRecordingSettings() {
    NSURL* url = [NSURL URLWithString:
        @"x-apple.systempreferences:com.apple.preference.security?Privacy_ScreenCapture"];
    [[NSWorkspace sharedWorkspace] openURL:url];
}

} // namespace trout
