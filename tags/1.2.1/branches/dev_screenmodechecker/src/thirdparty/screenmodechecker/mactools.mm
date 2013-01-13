#include "mactools.h"

#include <ApplicationServices/ApplicationServices.h>
#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>

@interface MacScreenMonitor : NSObject {
 @private
  BOOL screensaverRunning_;
  BOOL screenLocked_;
}

@property (readonly, nonatomic, getter=isScreensaverRunning) BOOL screensaverRunning;
@property (readonly, nonatomic, getter=isScreenLocked) BOOL screenLocked;

@end

@implementation MacScreenMonitor

@synthesize screensaverRunning = screensaverRunning_;
@synthesize screenLocked = screenLocked_;

- (id)init {
  if ((self = [super init])) {
	NSDistributedNotificationCenter* distCenter = [NSDistributedNotificationCenter defaultCenter];
	[distCenter addObserver:self
				   selector:@selector(onScreenSaverStarted:)
					   name:@"com.apple.screensaver.didstart"
					 object:nil];
	[distCenter addObserver:self
				   selector:@selector(onScreenSaverStopped:)
					   name:@"com.apple.screensaver.didstop"
					 object:nil];
	[distCenter addObserver:self
				   selector:@selector(onScreenLocked:)
					   name:@"com.apple.screenIsLocked"
					 object:nil];
	[distCenter addObserver:self
				   selector:@selector(onScreenUnlocked:)
					   name:@"com.apple.screenIsUnlocked"
					 object:nil];
  }
  return self;
}

- (void)dealloc {
  [[NSDistributedNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

- (void)onScreenSaverStarted:(NSNotification*)notification {
   screensaverRunning_ = YES;
}

- (void)onScreenSaverStopped:(NSNotification*)notification {
   screensaverRunning_ = NO;
}

- (void)onScreenLocked:(NSNotification*)notification {
   screenLocked_ = YES;
}

- (void)onScreenUnlocked:(NSNotification*)notification {
   screenLocked_ = NO;
}

@end

@interface FullScreenMonitor : NSObject {
 @private
  BOOL fullScreen_;
  EventHandlerRef eventHandler_;
}

@property (nonatomic, getter=isFullScreen) BOOL fullScreen;

@end

static OSStatus handleAppEvent(EventHandlerCallRef myHandler, EventRef event, void* userData) {

  FullScreenMonitor* fullScreenMonitor = reinterpret_cast<FullScreenMonitor*>(userData);

  UInt32 mode = 0;
  OSStatus status = GetEventParameter(event, kEventParamSystemUIMode, typeUInt32, NULL, sizeof(UInt32), NULL, &mode);
  if (status != noErr)
	return status;
  BOOL isFullScreenMode = mode == kUIModeAllHidden;
  [fullScreenMonitor setFullScreen:isFullScreenMode];
  return noErr;
}

@implementation FullScreenMonitor

@synthesize fullScreen = fullScreen_;

- (id)init {
  if ((self = [super init])) {
	SystemUIMode currentMode;
	GetSystemUIMode(&currentMode, NULL);
	fullScreen_ = currentMode == kUIModeAllHidden;

	EventTypeSpec events[] =
	  {{ kEventClassApplication, kEventAppSystemUIModeChanged }};
	OSStatus status = InstallApplicationEventHandler(
		NewEventHandlerUPP(handleAppEvent),
		GetEventTypeCount(events),
		events,
		self,
		&eventHandler_);
	if (status) {
	  [self release];
	  self = nil;
	}
  }
  return self;
}

- (void)dealloc {
  if (eventHandler_)
	RemoveEventHandler(eventHandler_);
  [super dealloc];
}

@end

static FullScreenMonitor* g_fullScreenMonitor = nil;
static MacScreenMonitor* g_screenMonitor = nil;

void InitTools() {
  if (!g_screenMonitor)
	g_screenMonitor = [[MacScreenMonitor alloc] init];
  if (!g_fullScreenMonitor)
	g_fullScreenMonitor = [[FullScreenMonitor alloc] init];
}

void StopTools() {
  [g_screenMonitor release];
  g_screenMonitor = nil;
  [g_fullScreenMonitor release];
  g_fullScreenMonitor = nil;
}

bool CheckStateIsLocked() {
  return [g_screenMonitor isScreensaverRunning] || [g_screenMonitor isScreenLocked];
}

bool IsFullScreenMode() {
  if (CGDisplayIsCaptured(CGMainDisplayID()))
	return true;

  return [g_fullScreenMonitor isFullScreen];
}
