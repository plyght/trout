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

static bool copyStringAttribute(AXUIElementRef element, CFStringRef attribute, std::string* out) {
    CFTypeRef value = nullptr;
    AXError err = AXUIElementCopyAttributeValue(element, attribute, &value);
    if (err != kAXErrorSuccess || !value) return false;
    bool ok = CFGetTypeID(value) == CFStringGetTypeID();
    if (ok && out) *out = cfToStd((CFStringRef)value);
    CFRelease(value);
    return ok;
}

static bool copySelectedTextRange(AXUIElementRef element, CFRange* out) {
    CFTypeRef rangeRef = nullptr;
    AXError err = AXUIElementCopyAttributeValue(element, kAXSelectedTextRangeAttribute, &rangeRef);
    if (err != kAXErrorSuccess || !rangeRef) return false;
    bool ok = CFGetTypeID(rangeRef) == AXValueGetTypeID() &&
        AXValueGetType((AXValueRef)rangeRef) == kAXValueCFRangeType &&
        AXValueGetValue((AXValueRef)rangeRef, (AXValueType)kAXValueCFRangeType, out);
    CFRelease(rangeRef);
    return ok;
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
    std::string role;
    std::string subrole;
    copyStringAttribute(el, kAXRoleAttribute, &role);
    copyStringAttribute(el, kAXSubroleAttribute, &subrole);

    if (role == "AXSecureTextField" || subrole == "AXSecureTextField") {
        ft.secure = true;
        CFRelease(el);
        return ft;
    }

    copyStringAttribute(el, kAXRoleDescriptionAttribute, &ft.roleDesc);

    // Value (full text).
    bool hasValue = copyStringAttribute(el, kAXValueAttribute, &ft.value);

    // Selected text range (caret position).
    CFRange r{0, 0};
    bool hasRange = copySelectedTextRange(el, &r);
    if (hasRange) {
        ft.selectionStart = r.location;
        ft.selectionLength = r.length;
    }

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

    ft.valid = !ft.bundleId.empty() && (hasValue || hasRange);
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
    CFRange r{0, 0};
    if (copySelectedTextRange(el, &r)) {
        // Use a 1-char range at the caret for bounds.
        CFRange caretRange{ r.location, 1 };
        AXValueRef caretRangeVal = AXValueCreate((AXValueType)kAXValueCFRangeType, &caretRange);
        CFTypeRef boundsRef = nullptr;
        AXError bErr = AXUIElementCopyParameterizedAttributeValue(
            el, kAXBoundsForRangeParameterizedAttribute, caretRangeVal, &boundsRef);
        if (bErr == kAXErrorSuccess && boundsRef &&
            CFGetTypeID(boundsRef) == AXValueGetTypeID() &&
            AXValueGetType((AXValueRef)boundsRef) == kAXValueCGRectType) {
            CGRect rect{};
            if (AXValueGetValue((AXValueRef)boundsRef, (AXValueType)kAXValueCGRectType, &rect)) {
                *outRect = rect; // top-left origin (AX/Quartz screen coords)
                ok = true;
            }
        }
        if (boundsRef) CFRelease(boundsRef);
        if (caretRangeVal) CFRelease(caretRangeVal);
    }
    CFRelease(el);
    return ok;
}

} // namespace trout
