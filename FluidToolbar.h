//
//  FluidToolbar.h
//  FluidApp
//

#import <Cocoa/Cocoa.h>


@interface FluidToolbar : NSObject
{
	IBOutlet NSView* header;
	IBOutlet NSView* basicTools;
	IBOutlet NSView* diffusionTools;
	IBOutlet NSView* qualityTools;
	
	IBOutlet NSView* headerContainer;
	
	IBOutlet NSButton *btnBasic;
	IBOutlet NSButton *btnDiffusion;
	IBOutlet NSButton *btnQuality;
	
	NSView *visibleView;
	
	NSArray *allItems;
}

- (IBAction)onPressButton:in_btn;

@end
