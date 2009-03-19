//
//  FluidApp_Delegate.h
//  FluidApp
//

#import <Cocoa/Cocoa.h>


@interface FluidApp_Delegate : NSObject
{
	IBOutlet NSWindow *i_window;
	IBOutlet NSWindow *i_textWindow;
	IBOutlet NSWindow *i_hostSheet;
	
	IBOutlet NSTextView *i_textView;
	
	IBOutlet NSTextField *i_netAddress;

	NSOpenGLView *r_view;
	NSOpenGLContext *m_context;
	NSTimer *r_timer;
}

- (IBAction)onSave:(id)in_src;
- (IBAction)onOpen:(id)in_src;

- (IBAction)onConnect:(id)in_src;
- (IBAction)onCancelConnect:(id)in_src;
- (IBAction)onDoConnect:(id)in_src;

@end
