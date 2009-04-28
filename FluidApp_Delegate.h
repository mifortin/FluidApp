//
//  FluidApp_Delegate.h
//  FluidApp
//

#import <Cocoa/Cocoa.h>

#include "protocol.h"
#import "X_SimpleNumber.h"

@interface FluidApp_Delegate : NSObject
{
	IBOutlet NSWindow *i_window;
	IBOutlet NSWindow *i_textWindow;
	IBOutlet NSWindow *i_hostSheet;
	
	IBOutlet NSTextView *i_textView;
	IBOutlet NSTextView *i_logView;
	
	IBOutlet NSTextField *i_netAddress;

	NSOpenGLView *r_view;
	NSOpenGLContext *m_context;
	NSTimer *r_timer;
	
	protocol *r_proto;
	
	IBOutlet NSView *i_sideBar;
	IBOutlet NSView *i_logBar;
	
	NSDrawer *r_drawer;
	
	NSMutableArray *allObjs;
	
	//These are loaded dynamically
	IBOutlet X_SimpleNumber *i_simpleNumber;
}

- (IBAction)onSave:(id)in_src;
- (IBAction)onOpen:(id)in_src;

- (IBAction)onConnect:(id)in_src;
- (IBAction)onCancelConnect:(id)in_src;
- (IBAction)onDoConnect:(id)in_src;

- (IBAction)clearLog:(id)srcObj;
@end
