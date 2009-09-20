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
	IBOutlet FluidAppGL	*ib_glView;
	IBOutlet NSWindow		*ib_window;
	IBOutlet NSView			*ib_toolbarView;
	
	IBOutlet NSSlider		*ib_sld_viscosity;
	IBOutlet NSTextField	*ib_txt_viscosity;
	
	NSTimer			*r_timer;
}

- (IBAction)onChangeViscosity:(id)value;

@end
