#import "AppController.h"
#import "MenuBarController.h"
#import "Overlay.h"
#import "Permissions.h"
#import "Accessibility.h"
#import "EventTap.h"
#import "SettingsWindow.h"

#include "trout/Paths.h"
#include "trout/Store.h"
#include "trout/Settings.h"
#include "trout/Engine.h"
#include "trout/Compat.h"
#include "trout/Emoji.h"
#include "trout/Models.h"
#include "trout/Log.h"

#import <Carbon/Carbon.h> // virtual keycodes (kVK_*)

#include <memory>
#include <string>
#include <atomic>

using namespace trout;

@implementation AppController {
    std::unique_ptr<Store> _store;
    std::unique_ptr<Settings> _settings;
    std::unique_ptr<Engine> _engine;
    std::unique_ptr<Compat> _compat;
    std::unique_ptr<Emoji> _emoji;
    std::unique_ptr<Models> _models;
    std::unique_ptr<Accessibility> _ax;
    std::unique_ptr<EventTap> _tap;

    MenuBarController* _menu;
    TroutOverlay* _overlay;
    SettingsWindowController* _settingsWin;

    // Current suggestion state (main thread only).
    std::string _suggestionText;     // remaining (unaccepted) suggestion
    std::vector<size_t> _wordEnds;   // word boundaries within _suggestionText
    uint64_t _currentRequestId;
    int _debounceToken;
}

- (instancetype)init {
    if ((self = [super init])) {
        _currentRequestId = 0;
        _debounceToken = 0;
    }
    return self;
}

- (void)startServices {
    Paths::instance().ensureDirectories();
    Log::instance().setLogFile(Paths::instance().logFile());
    TROUT_INFO("trout starting");

    _store = std::make_unique<Store>();
    _store->open(Paths::instance().databaseFile());

    _settings = std::make_unique<Settings>(_store.get());
    _settings->load();

    _compat = std::make_unique<Compat>();
    _emoji = std::make_unique<Emoji>();
    _models = std::make_unique<Models>(Paths::instance().modelsDir());
    _ax = std::make_unique<Accessibility>();

    _engine = std::make_unique<Engine>(_settings.get(), _store.get());
    std::string err;
    _engine->start(&err);

    __weak AppController* weakSelf = self;
    _engine->setSuggestionCallback([weakSelf](const Suggestion& s) {
        AppController* strong = weakSelf;
        if (!strong) return;
        // Marshal to main thread for UI.
        std::string text = s.fullText;
        std::vector<size_t> ends = s.wordEnds;
        uint64_t rid = s.requestId;
        bool typo = s.typoSuppressed;
        dispatch_async(dispatch_get_main_queue(), ^{
            [strong onSuggestion:text wordEnds:ends requestId:rid typo:typo];
        });
    });

    _overlay = [[TroutOverlay alloc] init];
    _menu = [[MenuBarController alloc] initWithApp:self];
    [_menu setVisible:_settings->global().showMenuBarIcon];

    // Request accessibility permission (prompts on first run).
    bool trusted = Permissions::accessibilityTrusted(true);
    if (!trusted) {
        TROUT_WARN("accessibility not trusted yet; typing integration disabled until granted");
    }

    [self startEventTap];

    if (!_settings->global().selectedModelId.size() || !trusted) {
        [self openSettings];
    }
}

- (void)startEventTap {
    _tap = std::make_unique<EventTap>();
    __weak AppController* weakSelf = self;
    bool ok = _tap->start([weakSelf](CGKeyCode key, CGEventFlags flags, bool isDown) -> bool {
        AppController* strong = weakSelf;
        if (!strong) return false;
        return [strong handleKey:key flags:flags isDown:isDown];
    });
    if (!ok) TROUT_WARN("event tap not started");
}

// Returns YES to swallow the key event.
- (bool)handleKey:(CGKeyCode)key flags:(CGEventFlags)flags isDown:(bool)isDown {
    if (!isDown) return false;
    if (![self completionsEnabled]) return false;

    const bool ctrl = (flags & kCGEventFlagMaskControl) != 0;
    const bool opt  = (flags & kCGEventFlagMaskAlternate) != 0;
    const bool cmd  = (flags & kCGEventFlagMaskCommand) != 0;
    const bool plainMods = !ctrl && !opt && !cmd;

    const bool haveSuggestion = !_suggestionText.empty() && [_overlay visible];

    // Force-activate completions: Ctrl + `
    if (key == kVK_ANSI_Grave && ctrl && !opt && !cmd) {
        [self requestCompletionNow];
        return true;
    }
    // Accept full completion: ` (backtick) when a suggestion is visible.
    if (key == kVK_ANSI_Grave && plainMods && haveSuggestion) {
        [self acceptFull];
        return true;
    }
    // Accept next word: Tab when a suggestion is visible.
    if (key == kVK_Tab && plainMods && haveSuggestion) {
        [self acceptNextWord];
        return true;
    }
    // Dismiss: Escape.
    if (key == kVK_Escape && haveSuggestion) {
        [self dismiss];
        return false; // let Escape continue to the app too
    }

    // Any other key: treat as typing. Schedule a debounced completion request.
    // Don't swallow.
    if (plainMods || flags == 0 ||
        ((flags & kCGEventFlagMaskShift) && !ctrl && !opt && !cmd)) {
        [self dismiss];
        [self scheduleCompletion];
    }
    return false;
}

- (void)scheduleCompletion {
    _debounceToken++;
    int token = _debounceToken;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(120 * NSEC_PER_MSEC)),
                   dispatch_get_main_queue(), ^{
        if (token != self->_debounceToken) return; // superseded
        [self requestCompletionNow];
    });
}

- (void)requestCompletionNow {
    if (!_engine->modelReady()) return;
    FocusedText ft = _ax->readFocused();
    if (!ft.valid || ft.secure) return;
    if (ft.textBeforeCursor.empty()) return;

    auto g = _settings->global();
    CompletionContext ctx;
    ctx.bundleId = ft.bundleId;
    ctx.textBeforeCursor = ft.textBeforeCursor;
    ctx.textAfterCursor = ft.textAfterCursor;
    ctx.isSecureField = ft.secure;
    if (g.useClipboardContext) {
        NSString* clip = [[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];
        if (clip) ctx.clipboardText = std::string(clip.UTF8String);
    }
    _currentRequestId = _engine->requestCompletion(ctx);
}

- (void)onSuggestion:(std::string)text
            wordEnds:(std::vector<size_t>)ends
           requestId:(uint64_t)rid
                typo:(bool)typo {
    if (rid != _currentRequestId) return;
    if (typo || text.empty()) { [self dismiss]; return; }
    _suggestionText = text;
    _wordEnds = ends;
    [self renderOverlay];
}

- (void)renderOverlay {
    if (_suggestionText.empty()) { [_overlay hide]; return; }
    NSString* ns = [NSString stringWithUTF8String:_suggestionText.c_str()];
    CGRect caret = CGRectNull;
    _ax->caretRect(&caret);
    [_overlay showText:ns acceptedLen:0 caretRect:caret];
}

- (void)acceptNextWord {
    if (_suggestionText.empty()) return;
    size_t cut = _wordEnds.empty() ? _suggestionText.size() : _wordEnds.front();
    if (cut == 0 || cut > _suggestionText.size()) cut = _suggestionText.size();
    std::string word = _suggestionText.substr(0, cut);

    if (_ax->insertText(word)) {
        int chars = (int)word.size();
        _engine->recordAcceptance(1, chars, _ax->frontmostBundleId());
    }

    // Advance remaining suggestion and rebase word boundaries.
    _suggestionText = _suggestionText.substr(cut);
    std::vector<size_t> rebased;
    for (size_t e : _wordEnds) if (e > cut) rebased.push_back(e - cut);
    _wordEnds = rebased;

    if (_suggestionText.empty()) { [self dismiss]; }
    else { [self renderOverlay]; }
}

- (void)acceptFull {
    if (_suggestionText.empty()) return;
    std::string text = _suggestionText;
    if (_ax->insertText(text)) {
        int words = (int)std::max<size_t>(1, _wordEnds.size());
        _engine->recordAcceptance(words, (int)text.size(), _ax->frontmostBundleId());
    }
    [self dismiss];
}

- (void)dismiss {
    _suggestionText.clear();
    _wordEnds.clear();
    [_overlay hide];
}

- (void)shutdown {
    if (_engine) _engine->stop();
    if (_tap) _tap->stop();
    if (_settings) _settings->save();
    if (_store) _store->close();
}

#pragma mark - Menu / settings API

- (BOOL)completionsEnabled { return _settings && _settings->global().completionsEnabled; }

- (void)setCompletionsEnabled:(BOOL)enabled {
    auto g = _settings->global();
    g.completionsEnabled = enabled;
    _settings->setGlobal(g);
    if (!enabled) [self dismiss];
    [_menu refresh];
}

- (void)openSettings {
    if (!_settingsWin) {
        _settingsWin = [[SettingsWindowController alloc] initWithApp:self];
    }
    [_settingsWin showWindow];
}

- (void)revealModelsFolder {
    NSString* dir = [NSString stringWithUTF8String:Paths::instance().modelsDir().c_str()];
    [[NSWorkspace sharedWorkspace] selectFile:nil inFileViewerRootedAtPath:dir];
}

- (NSString*)selectedModelDisplayName {
    auto g = _settings->global();
    if (g.selectedModelId.empty()) return @"No model";
    std::string id = g.selectedModelId;
    size_t slash = id.find_last_of('/');
    if (slash != std::string::npos) id = id.substr(slash + 1);
    return [NSString stringWithUTF8String:id.c_str()];
}

- (BOOL)accessibilityReady { return Permissions::accessibilityTrusted(false); }

- (void*)settingsPtr { return _settings.get(); }
- (void*)storePtr { return _store.get(); }
- (void*)modelsPtr { return _models.get(); }
- (void*)compatPtr { return _compat.get(); }

- (void)reloadEngineModel {
    std::string err;
    if (!_engine->reloadModel(&err)) {
        TROUT_WARN("reload model failed: " << err);
    }
    [_menu refresh];
}

@end
