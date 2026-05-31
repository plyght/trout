#include "EventTap.h"
#include "trout/Log.h"

#import <CoreGraphics/CoreGraphics.h>

namespace trout {

static CGEventRef tapCallback(CGEventTapProxy, CGEventType type,
                              CGEventRef event, void* userInfo) {
    auto* self = static_cast<EventTap*>(userInfo);
    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
        // Re-enable handled in handle(); return the event unchanged.
        self->handle(type, event);
        return event;
    }
    bool consume = self->handle(type, event);
    return consume ? nullptr : event;
}

EventTap::~EventTap() { stop(); }

bool EventTap::start(KeyHandler handler) {
    handler_ = std::move(handler);
    CGEventMask mask = CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp);
    tap_ = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap,
                            kCGEventTapOptionDefault, mask, tapCallback, this);
    if (!tap_) {
        TROUT_ERROR("failed to create event tap (accessibility permission?)");
        return false;
    }
    source_ = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap_, 0);
    CFRunLoopAddSource(CFRunLoopGetMain(), source_, kCFRunLoopCommonModes);
    CGEventTapEnable(tap_, true);
    TROUT_INFO("event tap started");
    return true;
}

void EventTap::stop() {
    if (tap_) { CGEventTapEnable(tap_, false); }
    if (source_) {
        CFRunLoopRemoveSource(CFRunLoopGetMain(), source_, kCFRunLoopCommonModes);
        CFRelease(source_);
        source_ = nullptr;
    }
    if (tap_) { CFRelease(tap_); tap_ = nullptr; }
}

bool EventTap::handle(CGEventType type, CGEventRef event) {
    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
        if (tap_) CGEventTapEnable(tap_, true);
        return false;
    }
    if (!handler_) return false;
    bool isDown = (type == kCGEventKeyDown);
    CGKeyCode key = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
    CGEventFlags flags = CGEventGetFlags(event);
    return handler_(key, flags, isDown);
}

} // namespace trout
