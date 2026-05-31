#pragma once

#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>

// A borderless, transparent, non-activating panel that renders ghost suggestion
// text near the caret. When caret geometry is unavailable the caller positions
// it as a mirror window near the mouse or screen center.
@interface TroutOverlay : NSObject

- (instancetype)init;

// Show ghost text. acceptedLen marks the prefix already accepted word-by-word
// (rendered dimmer/struck so the user sees progress). caretRect is in Quartz
// top-left screen coordinates; pass CGRectNull to use mirror placement.
- (void)showText:(NSString*)text
     acceptedLen:(NSUInteger)acceptedLen
       caretRect:(CGRect)caretRect;

- (void)hide;
- (BOOL)visible;

@end
