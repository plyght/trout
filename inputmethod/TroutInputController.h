#pragma once

#import <InputMethodKit/InputMethodKit.h>
#import <Cocoa/Cocoa.h>

// IMKInputController subclass for the Trout input source. Most keystrokes pass
// through to the client unchanged so typing latency is preserved. The Tab key
// is intercepted while a suggestion is active and broadcast to the main Trout
// agent via a distributed notification; the agent reads context and asks the
// controller to commit accepted text back into the client.
//
// Coordination notifications (DistributedNotificationCenter):
//   "app.trout.acceptWord"  posted by IMK on Tab keydown
//   "app.trout.acceptFull"  posted by IMK on backtick keydown
//   "app.trout.commitText"  observed by IMK; userInfo[@"text"] inserted to client
@interface TroutInputController : IMKInputController
@end
