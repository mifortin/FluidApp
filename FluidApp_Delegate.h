//
//  FluidApp_Delegate.h
//  FluidApp
//

#import <Cocoa/Cocoa.h>


@interface FluidApp_Delegate : NSObject
{
	IBOutlet NSWindow *i_window;

	NSOpenGLView *r_view;
	NSOpenGLContext *m_context;
	NSTimer *r_timer;
}

@end
