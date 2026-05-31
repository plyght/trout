#include "Accessibility.h"
#include "trout/Log.h"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>

namespace trout {

static std::string cfToStd(CFStringRef s) {
    if (!s) return "";
    CFIndex len = CFStringGetLength(s);
    CFIndex max = CFStringGetMaximumSizeForEncoding(len, kCFStringEncodingUTF8) + 1;
    std::string out(max, '\0');
    if (CFStringGetCString(s, out.data(), max, kCFStringEncodingUTF8))
        out.resize(strlen(out.c_str()));
    else
        out.clear();
    return out;
}

Accessibility::Accessibility() {
    systemWide_ = AXUIElementCreateSystemWide();
}

Accessibility::~Accessibility() {
    if (systemWide_) CFRelease(systemWide_);
}

AXUIElementRef Accessibility::copyFocusedElement() {
    CFTypeRef focused = nullptr;
    AXError err = AXUIElementCopyAttributeValue(
        systemWide_, kAXFocusedUIElementAttribute, &focused);
    if (err != kAXErrorSuccess || !focused) return nullptr;
    return (AXUIElementRef)focused;
}

std::string Accessibility::frontmostBundleId() {
    NSRunningApplication* app = [[NSWorkspace sharedWorkspace] frontmostApplication];
    if (!app) return "";
    NSString* bid = app.bundleIdentifier;
    return bid ? std::string(bid.UTF8String) : "";
}

FocusedText Accessibility::readFocused() {
    FocusedText ft;
    AXUIElementRef el = copyFocusedElement();
    if (!el) return ft;

    ft.bundleId = frontmostBundleId();

    // Role / subrole for secure-field detection.
    CFTypeRef roleRef = nullptr;
    AXUIElementCopyAttributeValue(el, kAXRoleAttribute, &roleRef);
    std::string role = roleRef ? cfToStd((CFStringRef)roleRef) : "";
    if (roleRef) CFRelease(roleRef);

    CFTypeRef subRef = nullptr;
    AXUIElementCopyAttributeValue(el, kAXSubroleAttribute, &subRef);
    std::string subrole = subRef ? cfToStd((CFStringRef)subRef) : "";
    if (subRef) CFRelease(subRef);

    if (subrole == "AXSecureTextField") {
        ft.secure = true;
        CFRelease(el);
        return ft;
    }

    CFTypeRef roleDescRef = nullptr;
    AXUIElementCopyAttributeValue(el, kAXRoleDescriptionAttribute, &roleDescRef);
    if (roleDescRef) { ft.roleDesc = cfToStd((CFStringRef)roleDescRef); CFRelease(roleDescRef); }

    // Value (full text).
    CFTypeRef valueRef = nullptr;
    AXError vErr = AXUIElementCopyAttributeValue(el, kAXValueAttribute, &valueRef);
    if (vErr == kAXErrorSuccess && valueRef &&
        CFGetTypeID(valueRef) == CFStringGetTypeID()) {
        ft.value = cfToStd((CFStringRef)valueRef);
    }
    if (valueRef) CFRelease(valueRef);

    // Selected text range (caret position).
    CFTypeRef rangeRef = nullptr;
    AXError rErr = AXUIElementCopyAttributeValue(el, kAXSelectedTextRangeAttribute, &rangeRef);
    if (rErr == kAXErrorSuccess && rangeRef &&
        CFGetTypeID(rangeRef) == AXValueGetTypeID()) {
        CFRange r{0, 0};
        if (AXValueGetValue((AXValueRef)rangeRef, (AXValueType)kAXValueCFRangeType, &r)) {
            ft.selectionStart = r.location;
            ft.selectionLength = r.length;
        }
    }
    if (rangeRef) CFRelease(rangeRef);

    CFRelease(el);

    // Compute text before/after cursor using UTF-16 indices on an NSString to
    // match the AX index space, then convert to UTF-8.
    NSString* full = [NSString stringWithUTF8String:ft.value.c_str()];
    if (full) {
        NSUInteger n = full.length;
        NSUInteger caret = (NSUInteger)std::max<long>(0, std::min<long>(ft.selectionStart, (long)n));
        ft.textBeforeCursor = std::string([[full substringToIndex:caret] UTF8String] ?: "");
        ft.textAfterCursor = std::string([[full substringFromIndex:caret] UTF8String] ?: "");
    } else {
        ft.textBeforeCursor = ft.value;
    }

    ft.valid = !ft.bundleId.empty();
    return ft;
}

bool Accessibility::insertText(const std::string& text) {
    AXUIElementRef el = copyFocusedElement();
    if (!el) return false;

    NSString* ns = [NSString stringWithUTF8String:text.c_str()];
    if (!ns) { CFRelease(el); return false; }

    // Preferred path: set selected text (inserts at caret / replaces selection).
    AXError err = AXUIElementSetAttributeValue(
        el, kAXSelectedTextAttribute, (__bridge CFStringRef)ns);

    CFRelease(el);
    return err == kAXErrorSuccess;
}

bool Accessibility::caretRect(CGRect* outRect) {
    if (!outRect) return false;
    AXUIElementRef el = copyFocusedElement();
    if (!el) return false;

    bool ok = false;
    CFTypeRef rangeRef = nullptr;
    if (AXUIElementCopyAttributeValue(el, kAXSelectedTextRangeAttribute, &rangeRef)
            == kAXErrorSuccess && rangeRef) {
        CFRange r{0, 0};
        AXValueGetValue((AXValueRef)rangeRef, (AXValueType)kAXValueCFRangeType, &r);
        // Use a 1-char range at the caret for bounds.
        CFRange caretRange{ r.location, 1 };
        AXValueRef caretRangeVal = AXValueCreate((AXValueType)kAXValueCFRangeType, &caretRange);
        CFTypeRef boundsRef = nullptr;
        AXError bErr = AXUIElementCopyParameterizedAttributeValue(
            el, kAXBoundsForRangeParameterizedAttribute, caretRangeVal, &boundsRef);
        if (bErr == kAXErrorSuccess && boundsRef) {
            CGRect rect{};
            if (AXValueGetValue((AXValueRef)boundsRef, (AXValueType)kAXValueCGRectType, &rect)) {
                *outRect = rect; // top-left origin (AX/Quartz screen coords)
                ok = true;
            }
            CFRelease(boundsRef);
        }
        if (caretRangeVal) CFRelease(caretRangeVal);
        CFRelease(rangeRef);
    }
    CFRelease(el);
    return ok;
}

} // namespace trout
