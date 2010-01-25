//
//  FluidToolbar.h
//  FluidApp
//

#import <Cocoa/Cocoa.h>
#import "FluidTabButton.h"


@interface FluidToolbar : NSObject
{
	IBOutlet NSView* header;
	IBOutlet NSView* basicTools;
	IBOutlet NSView* diffusionTools;
	IBOutlet NSView* qualityTools;
	IBOutlet NSView* clientTools;
	IBOutlet NSView* serverTools;
	IBOutlet NSView* temperature;
	
	IBOutlet NSView* headerContainer;
	
	IBOutlet FluidTabButton *btnBasic;
	IBOutlet FluidTabButton *btnDiffusion;
	IBOutlet FluidTabButton *btnQuality;
	IBOutlet FluidTabButton *btnClient;
	IBOutlet FluidTabButton *btnServer;
	IBOutlet FluidTabButton *btnTemperature;
	
	NSView *visibleView;
	
	NSArray *allItems;
}

- (IBAction)onPressButton:in_btn;

@end
