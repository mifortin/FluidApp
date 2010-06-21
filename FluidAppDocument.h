//
//  FluidAppDocument.h
//  FluidApp

#import <Cocoa/Cocoa.h>
#import "FluidAppGL.h"
#import "FluidFPSView.h"
#import "Select360.h"

@interface FluidAppDocument : NSDocument
{
	double m_prevTime;
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
	
	IBOutlet NSSlider		*ib_sld_pressureQuality;
	IBOutlet NSTextField	*ib_txt_pressureQuality;
	
	IBOutlet NSSlider		*ib_sld_viscosityQuality;
	IBOutlet NSTextField	*ib_txt_viscosityQuality;
	
	IBOutlet NSSlider		*ib_sld_fadeDensity;
	IBOutlet NSTextField	*ib_txt_fadeDensity;
	
	IBOutlet NSSlider		*ib_sld_fadeVelocity;
	IBOutlet NSTextField	*ib_txt_fadeVelocity;
	
	IBOutlet FluidFPSView	*ib_fpsView;
	
	IBOutlet Select360		*ib_gravDir;
	
	NSTimer			*r_timer;
}

- (IBAction)onChangeViscosity:(id)value;
- (IBAction)onChangeVorticity:(id)value;
- (IBAction)onChangeFadeDensity:(id)value;
- (IBAction)onChangeFadeVelocity:(id)value;

- (IBAction)onChangePressureQuality:(id)value;
- (IBAction)onChangeViscosityQuality:(id)value;

- (IBAction)onChangeFreeSurface:(id)value;

- (IBAction)onChangeVorticityQuality:(id)value;

- (IBAction)onChangeGravityMagnitude:(id)value;
- (IBAction)onChangeTemperatureMag:(id)value;

- (void)onSetVisual:(int)in_visual;

@end
