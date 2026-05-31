#import "Overlay.h"

@interface TroutOverlayView : NSView
@property(nonatomic, strong) NSAttributedString* attr;
@end

@implementation TroutOverlayView
- (BOOL)isFlipped { return YES; }
- (void)drawRect:(NSRect)dirtyRect {
    [[NSColor clearColor] set];
    NSRectFill(dirtyRect);

    NSRect bg = NSInsetRect(self.bounds, 0.5, 0.5);
    NSBezierPath* path = [NSBezierPath bezierPathWithRoundedRect:bg xRadius:6 yRadius:6];
    [[NSColor colorWithCalibratedWhite:0.12 alpha:0.92] set];
    [path fill];
    [[NSColor colorWithCalibratedWhite:1.0 alpha:0.12] set];
    [path stroke];

    if (self.attr) {
        [self.attr drawAtPoint:NSMakePoint(8, 5)];
    }
}
@end

@interface TroutOverlay ()
@property(nonatomic, strong) NSPanel* panel;
@property(nonatomic, strong) TroutOverlayView* view;
@end

@implementation TroutOverlay

- (instancetype)init {
    if ((self = [super init])) {
        NSRect frame = NSMakeRect(0, 0, 320, 28);
        self.panel = [[NSPanel alloc] initWithContentRect:frame
                                                styleMask:NSWindowStyleMaskBorderless
                                                  backing:NSBackingStoreBuffered
                                                    defer:NO];
        self.panel.opaque = NO;
        self.panel.backgroundColor = [NSColor clearColor];
        self.panel.hasShadow = YES;
        self.panel.level = NSStatusWindowLevel;
        self.panel.ignoresMouseEvents = YES;
        self.panel.floatingPanel = YES;
        self.panel.becomesKeyOnlyIfNeeded = YES;
        self.panel.collectionBehavior =
            NSWindowCollectionBehaviorCanJoinAllSpaces |
            NSWindowCollectionBehaviorStationary |
            NSWindowCollectionBehaviorIgnoresCycle;

        self.view = [[TroutOverlayView alloc] initWithFrame:frame];
        self.panel.contentView = self.view;
    }
    return self;
}

- (NSAttributedString*)buildAttr:(NSString*)text accepted:(NSUInteger)acceptedLen {
    NSFont* font = [NSFont systemFontOfSize:13 weight:NSFontWeightRegular];
    NSMutableAttributedString* s =
        [[NSMutableAttributedString alloc] initWithString:text];
    NSRange all = NSMakeRange(0, text.length);
    [s addAttribute:NSFontAttributeName value:font range:all];
    [s addAttribute:NSForegroundColorAttributeName
              value:[NSColor colorWithCalibratedWhite:0.85 alpha:1.0]
              range:all];
    if (acceptedLen > 0 && acceptedLen <= text.length) {
        NSRange acc = NSMakeRange(0, acceptedLen);
        [s addAttribute:NSForegroundColorAttributeName
                  value:[NSColor colorWithCalibratedWhite:0.5 alpha:1.0]
                  range:acc];
    }
    return s;
}

- (void)showText:(NSString*)text
     acceptedLen:(NSUInteger)acceptedLen
       caretRect:(CGRect)caretRect {
    if (text.length == 0) { [self hide]; return; }

    NSAttributedString* attr = [self buildAttr:text accepted:acceptedLen];
    self.view.attr = attr;

    NSSize sz = [attr size];
    CGFloat w = MIN(MAX(sz.width + 16, 60), 600);
    CGFloat h = sz.height + 10;

    NSPoint origin;
    if (!CGRectIsNull(caretRect)) {
        // Convert Quartz top-left to Cocoa bottom-left using the screen holding
        // the caret point.
        NSScreen* screen = [NSScreen mainScreen];
        for (NSScreen* s in [NSScreen screens]) {
            NSRect f = s.frame;
            CGFloat flippedY = NSMaxY([NSScreen screens][0].frame) - caretRect.origin.y;
            if (NSPointInRect(NSMakePoint(caretRect.origin.x, flippedY), f)) { screen = s; break; }
        }
        CGFloat totalTop = NSMaxY([NSScreen screens][0].frame);
        CGFloat cocoaY = totalTop - caretRect.origin.y - caretRect.size.height;
        origin = NSMakePoint(caretRect.origin.x, cocoaY - h - 2);
        (void)screen;
    } else {
        NSRect vis = [NSScreen mainScreen].visibleFrame;
        origin = NSMakePoint(NSMidX(vis) - w / 2, NSMidY(vis));
    }

    [self.panel setFrame:NSMakeRect(origin.x, origin.y, w, h) display:YES];
    [self.view setFrame:NSMakeRect(0, 0, w, h)];
    [self.view setNeedsDisplay:YES];
    [self.panel orderFrontRegardless];
}

- (void)hide {
    [self.panel orderOut:nil];
    self.view.attr = nil;
}

- (BOOL)visible { return self.panel.isVisible; }

@end
