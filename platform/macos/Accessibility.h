#pragma once

#include <string>
#include <cstdint>

#import <CoreGraphics/CoreGraphics.h>
#import <ApplicationServices/ApplicationServices.h>

namespace trout {

// A snapshot of the currently focused text field, read via the Accessibility API.
struct FocusedText {
    bool valid = false;
    bool secure = false;          // password/secure field -> never complete
    std::string bundleId;
    std::string roleDesc;
    std::string value;            // full field text (may be empty for some apps)
    long selectionStart = 0;      // UTF-16 index of caret / selection start
    long selectionLength = 0;
    std::string textBeforeCursor;
    std::string textAfterCursor;
};

// Reads and mutates the focused text element of the frontmost app. All methods
// must be called on the main thread.
class Accessibility {
public:
    Accessibility();
    ~Accessibility();

    // Snapshot the focused field. Returns invalid FocusedText if none/secure.
    FocusedText readFocused();

    // Insert text at the caret (replacing any selection). Returns success.
    bool insertText(const std::string& text);

    // Screen rect of the caret/selection for overlay placement. Returns false
    // if geometry is unavailable (caller should fall back to a mirror window).
    bool caretRect(CGRect* outRect);

    // Bundle id of the frontmost application.
    std::string frontmostBundleId();

private:
    AXUIElementRef systemWide_ = nullptr;
    AXUIElementRef copyFocusedElement();
};

} // namespace trout
