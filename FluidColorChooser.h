//
//  FluidColorChooser.h
//  FluidApp
//

#import <Cocoa/Cocoa.h>
#import "FluidTransparentWindow.h"

@protocol FluidColorChooserResponder
	-(void)onChangeColorRed:(float)in_r green:(float)in_g blue:(float)in_b;
@end

@interface FluidColorChooser : FluidTransparentWindow
{
	IBOutlet NSColorWell *ib_color;
	
	IBOutlet NSSlider *ib_red;
	IBOutlet NSSlider *ib_green;
	IBOutlet NSSlider *ib_blue;
	
	id<FluidColorChooserResponder> responder;
}

+ (FluidColorChooser*)sharedInstance;

- (void)setColorRed:(float)in_r green:(float)in_g blue:(float)in_b;

- (void)setResponder:(id<FluidColorChooserResponder>)in_r;

- (IBAction)onChangeRed:in_sld;
- (IBAction)onChangeGreen:in_sld;
- (IBAction)onChangeBlue:in_sld;

@end
