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
	
	IBOutlet NSView* headerContainer;
	
	IBOutlet FluidTabButton *btnBasic;
	IBOutlet FluidTabButton *btnDiffusion;
	IBOutlet FluidTabButton *btnQuality;
	
	NSView *visibleView;
	
	NSArray *allItems;
}

- (IBAction)onPressButton:in_btn;

@end
