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
	
	
	IBOutlet NSSlider		*ib_sld_vorticity;
	IBOutlet NSTextField	*ib_txt_vorticity;
	
	IBOutlet NSSlider		*ib_sld_timestep;
	IBOutlet NSTextField	*ib_txt_timestep;
	
	IBOutlet NSSlider		*ib_sld_fadeDensity;
	IBOutlet NSTextField	*ib_txt_fadeDensity;
	
	IBOutlet NSSlider		*ib_sld_fadeVelocity;
	IBOutlet NSTextField	*ib_txt_fadeVelocity;
	
	NSTimer			*r_timer;
}

- (IBAction)onChangeViscosity:(id)value;
- (IBAction)onChangeVorticity:(id)value;
- (IBAction)onChangeTimestep:(id)value;
- (IBAction)onChangeFadeDensity:(id)value;
- (IBAction)onChangeFadeVelocity:(id)value;

- (IBAction)onChangeFreeSurface:(id)value;

@end
