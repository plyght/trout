#pragma once

#import <AppKit/AppKit.h>

@class AppController;

// Hosts a Dear ImGui interface (Metal backend) inside an ordinary NSWindow for
// settings, model management, per-app profiles, stats, and the permissions
// checklist. Created lazily by AppController.
@interface SettingsWindowController : NSObject

- (instancetype)initWithApp:(AppController*)app;
- (void)showWindow;

@end
