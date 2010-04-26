//
//  FluidVisualController.h
//  FluidApp
//

#import <Cocoa/Cocoa.h>
#import "FluidGradient.h"

@class FluidAppDocument;

@interface FluidVisualController : NSObject
{
	IBOutlet NSView*		visualDefaultDensity;
	IBOutlet NSView*		visualTemperature;
	IBOutlet NSView*		visualVelocity;
	
	IBOutlet NSView*		container;
	
	IBOutlet NSPopUpButton*	mode;
	
	IBOutlet FluidAppDocument* eh;
	
	NSView *visibleView;
	
	NSArray *allItems;
	
	
	//Related to picking colors...
	IBOutlet FluidGradient *temperature;
}

- (IBAction)onChangeMode:in_mode;

- (u128f*)temperatureGradient;

@end
