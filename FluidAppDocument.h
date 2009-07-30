//
//  FluidAppDocument.h
//  FluidApp

#import <Cocoa/Cocoa.h>
#import "FluidAppGL.h"

@interface FluidAppDocument : NSDocument
{
	NSToolbar		*r_toolbar;
	NSArray			*r_toolbarItems;
	NSToolbarItem	*r_toolbarItem;
	IBOutlet NSOpenGLView	*ib_glView;
	IBOutlet NSWindow		*ib_window;
	IBOutlet NSView			*ib_toolbarView;
	
	NSTimer			*r_timer;
	FluidAppGL		*r_gl;
}

@end
