#pragma once

#import <AppKit/AppKit.h>

@class AppController;

// Owns the NSStatusItem and its menu. Reflects and toggles global state through
// the AppController.
@interface MenuBarController : NSObject

- (instancetype)initWithApp:(AppController*)app;
- (void)setVisible:(BOOL)visible;
- (void)refresh;

@end
