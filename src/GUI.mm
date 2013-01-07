#if !(TARGET_OS_IPHONE)

#include <Carbon/Carbon.h>
#include <CoreAudioKit/CoreAudioKit.h>
#include <AudioUnit/AUCocoaUIView.h>
#include <AudioUnit/AudioUnitCarbonView.h>
#include <iostream>
#include "AudioUnitUtils.h"

#pragma mark Objective-C

pascal OSStatus CarbonWindowEventHandler(EventHandlerCallRef nextHandlerRef, EventRef event, void * userData);

typedef enum {
	WindowTypeCocoa,
	WindowTypeCarbon
}
WindowType;

@interface AUGUIWindow : NSWindow<NSWindowDelegate>
{
	NSView * _AUView;
	WindowType _windowType;
	void * _windowLink;
	
	// These are only used if the Audio Unit requires a Carbon view
	WindowRef _carbonWindow;
	AudioUnitCarbonView _carbonView;
	EventHandlerRef _carbonEventHandler;
}

+ (BOOL) audioUnitHasCocoaView:(AudioUnit)unit;
+ (BOOL) audioUnitHasCarbonView:(AudioUnit)unit;

- (void) initWithCocoaViewForUnit:(AudioUnit)unit;
- (void) initWithCarbonViewForUnit:(AudioUnit)unit;
- (void) initWithGenericViewForUnit:(AudioUnit)unit;
- (void) initWithAudioUnitCocoaView:(NSView *)audioUnitView;

- (id) initWithAudioUnit:(AudioUnit)unit forceGeneric:(BOOL)useGeneric;

- (void) audioUnitChangedViewSize:(NSNotification *)notification;
- (void) removeNotifications;

@property (nonatomic, readonly) WindowRef carbonWindow;

@end

@implementation AUGUIWindow

@synthesize carbonWindow = _carbonWindow;

- (void) dealloc
{
	if(_carbonView)   CloseComponent(_carbonView);
	if(_carbonWindow) DisposeWindow(_carbonWindow);
	
	[_AUView release];
	_AUView = nil;

	[super dealloc];
}

- (id) initWithAudioUnit:(AudioUnit)unit forceGeneric:(BOOL)useGeneric
{
	if(useGeneric) {
		[self initWithGenericViewForUnit:unit];
	} else if([AUGUIWindow audioUnitHasCocoaView:unit]) {
		[self initWithCocoaViewForUnit:unit];
	} else if([AUGUIWindow audioUnitHasCarbonView:unit]) {
		[self initWithCarbonViewForUnit:unit];
	} else {
		[self initWithGenericViewForUnit:unit];
	}
	
	[self setDelegate:self];
	
	return self;
}

#pragma mark - Cocoa

- (void) initWithCocoaViewForUnit:(AudioUnit)unit
{
	// getting the size of the AU View info
	UInt32  dataSize;
	Boolean isWriteable;
	OSStatus result = AudioUnitGetPropertyInfo(unit,
											   kAudioUnitProperty_CocoaUI,
											   kAudioUnitScope_Global,
											   0,
											   &dataSize,
											   &isWriteable);
	
	UInt32 numberOfClasses = (dataSize - sizeof(CFURLRef)) / sizeof(CFStringRef);
	
	// getting the location / name of the necessary view factory bits
	CFURLRef cocoaViewBundlePath = NULL;
	CFStringRef factoryClassName = NULL;
	AudioUnitCocoaViewInfo * cocoaViewInfo = NULL;
	if((result == noErr) && (numberOfClasses > 0)) {
		cocoaViewInfo = (AudioUnitCocoaViewInfo *)malloc(dataSize);
		if(AudioUnitGetProperty(unit,
								kAudioUnitProperty_CocoaUI,
								kAudioUnitScope_Global,
								0,
								cocoaViewInfo,
								&dataSize) == noErr) {
			cocoaViewBundlePath = cocoaViewInfo->mCocoaAUViewBundleLocation;
			factoryClassName    = cocoaViewInfo->mCocoaAUViewClass[0];
		} else if(cocoaViewInfo != NULL) {
			free(cocoaViewInfo);
			cocoaViewInfo = NULL;
		}
	}
	
	// if we have everything we need, create the custom Cocoa view
	NSView * AUView = nil;
	if(cocoaViewBundlePath && factoryClassName)
	{
		NSBundle * viewBundle = [NSBundle bundleWithURL:(NSURL *)cocoaViewBundlePath];
		if(viewBundle)
		{
			Class factoryClass = [viewBundle classNamed:(NSString *)factoryClassName];
			id<AUCocoaUIBase> factoryInstance = [[[factoryClass alloc] init] autorelease];
			AUView = [[factoryInstance uiViewForAudioUnit:unit
												 withSize:NSMakeSize(0, 0)] retain];
			// cleanup
			CFRelease(cocoaViewBundlePath);
			if(cocoaViewInfo)
			{
				for(int i = 0; i < numberOfClasses; i++)
					CFRelease(cocoaViewInfo->mCocoaAUViewClass[i]);
				free(cocoaViewInfo);
			}
		}
	}
	
	[self initWithAudioUnitCocoaView:AUView];
}

- (void) initWithGenericViewForUnit:(AudioUnit)unit
{
	AUGenericView * AUView = [[AUGenericView alloc] initWithAudioUnit:unit];
	[AUView setShowsExpertParameters:YES];
	[self initWithAudioUnitCocoaView:AUView];
}

- (void) initWithAudioUnitCocoaView:(NSView *)audioUnitView
{
	_AUView = audioUnitView;
	NSRect contentRect = NSMakeRect(0, 0, audioUnitView.frame.size.width, audioUnitView.frame.size.height);
	self = [super initWithContentRect:contentRect
							styleMask:(NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask)
							  backing:NSBackingStoreBuffered
								defer:NO];
	if(self) {
		self.level = NSNormalWindowLevel;
		self.contentView = audioUnitView;
		_windowType = WindowTypeCocoa;
		
		[[NSNotificationCenter defaultCenter] addObserver:self
												 selector:@selector(audioUnitChangedViewSize:)
													 name:NSViewFrameDidChangeNotification
												   object:_AUView];
	}
}

#pragma mark - Carbon

- (void) initWithCarbonViewForUnit:(AudioUnit)unit
{	
	// This technique embeds a carbon window inside of a cocoa
	// window wrapper, as described by Technical Note TN2213
	// ("Audio Units: Embedding a Carbon View in a Cocoa Window")
	
	// Getting carbon view description
	UInt32 dataSize;
	Boolean isWriteable;
	
	RETURN_IF_ERR(AudioUnitGetPropertyInfo(unit,
										   kAudioUnitProperty_GetUIComponentList,
										   kAudioUnitScope_Global,
										   0,
										   &dataSize,
										   &isWriteable),
				  "getting size of carbon view info");
	
	ComponentDescription * desc = (ComponentDescription *)malloc(dataSize);
	
	RETURN_IF_ERR(AudioUnitGetProperty(unit,
									   kAudioUnitProperty_GetUIComponentList,
									   kAudioUnitScope_Global,
									   0,
									   desc,
									   &dataSize),
				  "getting carbon view info");
	
	ComponentDescription d = desc[0];
	
	// Creating carbon view component
	Component comp = FindNextComponent(NULL, &d);
	RETURN_IF_ERR(OpenAComponent(comp, &_carbonView), "opening carbon view component");
	
	// Creating a carbon window for the view
	Rect carbonWindowBounds = {100,100,300,300};
	RETURN_IF_ERR(CreateNewWindow(kPlainWindowClass,
								  (kWindowStandardHandlerAttribute |
								   kWindowCompositingAttribute),
								  &carbonWindowBounds,
								  &_carbonWindow),
				  "creating carbon window");
	
	// Creating carbon controls
	ControlRef rootControl, viewPane;
	RETURN_IF_ERR(GetRootControl(_carbonWindow, &rootControl), "getting root control of carbon window");
	
	// Creating the view
	Float32Point size = {0,0};
	Float32Point location = {0,0};
	RETURN_IF_ERR(AudioUnitCarbonViewCreate(_carbonView,
											unit,
											_carbonWindow,
											rootControl,
											&location,
											&size,
											&viewPane),
				  "creating carbon view for audio unit");
	
	// Putting everything in place
	GetControlBounds(viewPane, &carbonWindowBounds);
	size.x = carbonWindowBounds.right  - carbonWindowBounds.left;
	size.y = carbonWindowBounds.bottom - carbonWindowBounds.top;
	SizeWindow(_carbonWindow, (short) (size.x + 0.5), (short) (size.y + 0.5), true);
	ShowWindow(_carbonWindow);
	
	// Listening to window events
	EventTypeSpec windowEventTypes[] = {
		{kEventClassWindow, kEventWindowGetClickActivation},
		{kEventClassWindow, kEventWindowHandleDeactivate}
	};
	
	EventHandlerUPP ehUPP = NewEventHandlerUPP(CarbonWindowEventHandler);
	RETURN_IF_ERR(InstallWindowEventHandler(_carbonWindow,
											ehUPP,
											sizeof(windowEventTypes) / sizeof(EventTypeSpec),
											windowEventTypes,
											self,
											&_carbonEventHandler),
				  "setting up carbon window event handler");
	
	NSWindow * wrapperWindow = [[[NSWindow alloc] initWithWindowRef:_carbonWindow] autorelease];
	
	self = [super initWithContentRect:NSMakeRect(0, 0, size.x + 1, size.y + 1)
							styleMask:(NSClosableWindowMask | 
									   NSMiniaturizableWindowMask |
									   NSTitledWindowMask)
							  backing:NSBackingStoreBuffered
								defer:NO];
	
	if(self) {
		[wrapperWindow setFrameOrigin:self.frame.origin];
		[self addChildWindow:wrapperWindow ordered:NSWindowAbove];
		self.level = NSNormalWindowLevel;
		_windowType = WindowTypeCarbon;
	}
}

pascal OSStatus CarbonWindowEventHandler(EventHandlerCallRef nextHandlerRef,
										 EventRef event,
										 void * userData)
{
	AUGUIWindow * uiWindow = (AUGUIWindow *)userData;
	UInt32 eventKind = GetEventKind(event);
	switch (eventKind)  {
		case kEventWindowHandleDeactivate:
			ActivateWindow([uiWindow carbonWindow], true);
			return noErr;
			
		case kEventWindowGetClickActivation:
			[uiWindow makeKeyAndOrderFront:nil];
			ClickActivationResult howToHandleClick = kActivateAndHandleClick;
			SetEventParameter(event, 
							  kEventParamClickActivation,
							  typeClickActivationResult,
							  sizeof(ClickActivationResult),
							  &howToHandleClick);
			return noErr;
	}
	
	return eventNotHandledErr;
}

#pragma mark - Util

+ (BOOL) audioUnitHasCocoaView:(AudioUnit)unit
{
	UInt32 dataSize;
	UInt32 numberOfClasses;
	Boolean isWriteable;
	
	OSStatus result = AudioUnitGetPropertyInfo(unit,
											   kAudioUnitProperty_CocoaUI,
											   kAudioUnitScope_Global,
											   0,
											   &dataSize,
											   &isWriteable);
	
	numberOfClasses = (dataSize - sizeof(CFURLRef)) / sizeof(CFStringRef);
	
	return (result == noErr) && (numberOfClasses > 0);
}

+ (BOOL) audioUnitHasCarbonView:(AudioUnit)unit
{
	UInt32 dataSize;
	Boolean isWriteable;
	OSStatus s = AudioUnitGetPropertyInfo(unit,
										  kAudioUnitProperty_GetUIComponentList,
										  kAudioUnitScope_Global,
										  0,
										  &dataSize, 
										  &isWriteable);
	
	return (s == noErr) && (dataSize >= sizeof(ComponentDescription));
}

- (void) audioUnitChangedViewSize:(NSNotification *)notification
{
	[[NSNotificationCenter defaultCenter] removeObserver:self 
													name:NSViewFrameDidChangeNotification 
												  object:((NSView *)notification.object)];
	
	NSRect newRect = self.frame;
	NSSize newSize = [self frameRectForContentRect:((NSView *)notification.object).frame].size;
	newRect.origin.y -= newSize.height - newRect.size.height;
	newRect.size = newSize;
	[self setFrame:newRect display:YES];
	
	[[NSNotificationCenter defaultCenter] addObserver:self 
											 selector:@selector(audioUnitChangedViewSize:) 
												 name:NSViewFrameDidChangeNotification
											   object:((NSView *)notification.object)];
}

- (void) removeNotifications
{
	switch (_windowType) {
		case WindowTypeCocoa:
			[[NSNotificationCenter defaultCenter] removeObserver:self
															name:NSViewFrameDidChangeNotification
														  object:_AUView];
			break;
			
		case WindowTypeCarbon:
			RemoveEventHandler(_carbonEventHandler);
			break;
	}
}

#pragma mark - NSWindowDelegate

- (void) windowWillClose:(NSNotification *)notification
{
	[self removeNotifications];
}

@end

#pragma mark - C++

#include "GenericUnit.h"

using namespace cinder::audiounit;
using namespace std;

void GenericUnit::showUI(const string &title, UInt32 x, UInt32 y, bool forceGeneric)
{	
	NSString * windowTitle = [NSString stringWithUTF8String:title.c_str()];
	dispatch_async(dispatch_get_main_queue(), ^{

		AUGUIWindow * auWindow = [[AUGUIWindow alloc] initWithAudioUnit:*_unit
														   forceGeneric:forceGeneric];
		
		UInt32 flippedY = (UInt32)[[NSScreen mainScreen] visibleFrame].size.height - y - (UInt32)auWindow.frame.size.height;
		
		[auWindow setFrameOrigin:NSMakePoint(x, flippedY)];
		[auWindow setTitle:windowTitle];
		[auWindow makeKeyAndOrderFront:nil];
	});
}

#endif //TARGET_OS_IPHONE
