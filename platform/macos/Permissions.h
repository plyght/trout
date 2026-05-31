#pragma once

#include <functional>

namespace trout {

// Thin wrappers around macOS permission checks used by the setup checklist.
class Permissions {
public:
    // Accessibility (AXIsProcessTrusted). If prompt is true, shows the system
    // prompt directing the user to System Settings.
    static bool accessibilityTrusted(bool prompt);

    // Screen Recording (CGPreflightScreenCaptureAccess). Optional feature.
    static bool screenRecordingGranted();
    static void requestScreenRecording();

    // Opens the relevant System Settings privacy pane.
    static void openAccessibilitySettings();
    static void openScreenRecordingSettings();
};

} // namespace trout
