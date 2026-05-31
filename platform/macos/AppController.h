#pragma once

#import <AppKit/AppKit.h>

#include <memory>

namespace trout {
class Store;
class Settings;
class Engine;
class Compat;
class Emoji;
class Models;
class Accessibility;
class EventTap;
}

// Central coordinator for the menu-bar app. Owns the core engine and all macOS
// integration objects, and wires typing -> context -> completion -> overlay ->
// acceptance. Lives for the lifetime of the process.
@interface AppController : NSObject

- (instancetype)init;
- (void)startServices;
- (void)shutdown;

// Menu actions / state used by MenuBarController and the settings window.
- (BOOL)completionsEnabled;
- (void)setCompletionsEnabled:(BOOL)enabled;
- (void)openSettings;
- (void)revealModelsFolder;
- (NSString*)selectedModelDisplayName;
- (BOOL)accessibilityReady;

// Accessors for the settings window (returns trout::Settings*).
- (void*)settingsPtr;
- (void*)storePtr;
- (void*)modelsPtr;
- (void*)compatPtr;
- (void)reloadEngineModel;

@end
