#import "MenuBarController.h"
#import "AppController.h"

@implementation MenuBarController {
    NSStatusItem* _item;
    __weak AppController* _app;
    NSMenuItem* _toggleItem;
    NSMenuItem* _modelItem;
}

- (instancetype)initWithApp:(AppController*)app {
    if ((self = [super init])) {
        _app = app;
    }
    return self;
}

- (void)setVisible:(BOOL)visible {
    if (visible && !_item) {
        _item = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
        _item.button.title = @"\U0001F41F"; // fish
        _item.button.toolTip = @"Trout";
        [self buildMenu];
    } else if (!visible && _item) {
        [[NSStatusBar systemStatusBar] removeStatusItem:_item];
        _item = nil;
    }
}

- (void)buildMenu {
    NSMenu* menu = [[NSMenu alloc] init];

    _toggleItem = [[NSMenuItem alloc] initWithTitle:@"Enable Completions"
                                             action:@selector(toggleCompletions:)
                                      keyEquivalent:@""];
    _toggleItem.target = self;
    [menu addItem:_toggleItem];

    [menu addItem:[NSMenuItem separatorItem]];

    _modelItem = [[NSMenuItem alloc] initWithTitle:@"Model: \u2014" action:nil keyEquivalent:@""];
    _modelItem.enabled = NO;
    [menu addItem:_modelItem];

    NSMenuItem* settings = [[NSMenuItem alloc] initWithTitle:@"Settings\u2026"
                                                      action:@selector(openSettings:)
                                               keyEquivalent:@","];
    settings.target = self;
    [menu addItem:settings];

    NSMenuItem* reveal = [[NSMenuItem alloc] initWithTitle:@"Reveal Models Folder"
                                                    action:@selector(revealModels:)
                                             keyEquivalent:@""];
    reveal.target = self;
    [menu addItem:reveal];

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* quit = [[NSMenuItem alloc] initWithTitle:@"Quit Trout"
                                                  action:@selector(quit:)
                                           keyEquivalent:@"q"];
    quit.target = self;
    [menu addItem:quit];

    _item.menu = menu;
    [self refresh];
}

- (void)refresh {
    if (!_item) return;
    _toggleItem.state = [_app completionsEnabled] ? NSControlStateValueOn : NSControlStateValueOff;
    _modelItem.title = [NSString stringWithFormat:@"Model: %@", [_app selectedModelDisplayName]];
}

- (void)toggleCompletions:(id)sender {
    [_app setCompletionsEnabled:![_app completionsEnabled]];
    [self refresh];
}

- (void)openSettings:(id)sender { [_app openSettings]; }
- (void)revealModels:(id)sender { [_app revealModelsFolder]; }
- (void)quit:(id)sender { [NSApp terminate:nil]; }

@end
