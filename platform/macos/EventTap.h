#pragma once

#include <functional>

#import <CoreGraphics/CoreGraphics.h>

namespace trout {

// A session-level Quartz event tap for keyDown events. The handler returns true
// to swallow the event (e.g. when accepting a suggestion with Tab), false to
// let it pass through. The tap auto-re-enables if macOS disables it on timeout.
class EventTap {
public:
    using KeyHandler = std::function<bool(CGKeyCode keyCode, CGEventFlags flags, bool isKeyDown)>;

    EventTap() = default;
    ~EventTap();

    bool start(KeyHandler handler);
    void stop();
    bool running() const { return tap_ != nullptr; }

    // Internal: invoked by the C callback trampoline.
    bool handle(CGEventType type, CGEventRef event);

private:
    CFMachPortRef tap_ = nullptr;
    CFRunLoopSourceRef source_ = nullptr;
    KeyHandler handler_;
};

} // namespace trout
