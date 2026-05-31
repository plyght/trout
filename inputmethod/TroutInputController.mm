#import "TroutInputController.h"

#import <Carbon/Carbon.h>

static NSString* const kAcceptWord = @"app.trout.acceptWord";
static NSString* const kAcceptFull = @"app.trout.acceptFull";
static NSString* const kCommitText = @"app.trout.commitText";

@implementation TroutInputController {
    id _currentClient;
    BOOL _suggestionActive;
}

- (instancetype)initWithServer:(IMKServer*)server
                      delegate:(id)delegate
                        client:(id)inputClient {
    if ((self = [super initWithServer:server delegate:delegate client:inputClient])) {
        [[NSDistributedNotificationCenter defaultCenter]
            addObserver:self
               selector:@selector(onCommitText:)
                   name:kCommitText
                 object:nil];
        // The agent toggles this when a suggestion is on screen.
        [[NSDistributedNotificationCenter defaultCenter]
            addObserver:self
               selector:@selector(onSuggestionState:)
                   name:@"app.trout.suggestionState"
                 object:nil];
    }
    return self;
}

- (void)dealloc {
    [[NSDistributedNotificationCenter defaultCenter] removeObserver:self];
}

- (void)onSuggestionState:(NSNotification*)note {
    _suggestionActive = [note.userInfo[@"active"] boolValue];
}

- (void)onCommitText:(NSNotification*)note {
    NSString* text = note.userInfo[@"text"];
    if (text.length == 0 || !_currentClient) return;
    NSRange replace = NSMakeRange(NSNotFound, 0);
    [(id<IMKTextInput>)_currentClient insertText:text replacementRange:replace];
}

// Pass typed characters straight through to the client (commit immediately).
- (BOOL)inputText:(NSString*)string client:(id)sender {
    _currentClient = sender;
    [(id<IMKTextInput>)sender insertText:string
                       replacementRange:NSMakeRange(NSNotFound, 0)];
    return YES;
}

- (BOOL)handleEvent:(NSEvent*)event client:(id)sender {
    _currentClient = sender;
    if (event.type != NSEventTypeKeyDown) return NO;

    unsigned short code = event.keyCode;
    NSEventModifierFlags mods = event.modifierFlags &
        (NSEventModifierFlagControl | NSEventModifierFlagOption | NSEventModifierFlagCommand);

    if (code == kVK_Tab && mods == 0 && _suggestionActive) {
        [[NSDistributedNotificationCenter defaultCenter]
            postNotificationName:kAcceptWord object:nil userInfo:nil
              deliverImmediately:YES];
        return YES; // consume Tab
    }
    if (code == kVK_ANSI_Grave && mods == 0 && _suggestionActive) {
        [[NSDistributedNotificationCenter defaultCenter]
            postNotificationName:kAcceptFull object:nil userInfo:nil
              deliverImmediately:YES];
        return YES;
    }
    return NO; // let everything else pass through
}

@end
