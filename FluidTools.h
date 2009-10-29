//
//  FluidTools.h
//  FluidApp
//

#import <Cocoa/Cocoa.h>


@interface FluidTools : NSObject
{
	IBOutlet NSButton *btnDensity;
	IBOutlet NSButton *btnVelocity;
	IBOutlet NSButton *btnSource;
	
	NSArray *r_array;
	
	NSButton *btnSelected;
	
	IBOutlet NSSlider *brushSize;
	
	IBOutlet NSButton *viewDensity;
	IBOutlet NSButton *viewVelocity;
	NSArray *r_view;
	NSButton *viewSelected;
	
	IBOutlet NSPanel *pnl_view;
	IBOutlet NSPanel *pnl_brush;
}

- (IBAction)onButtonPress:in_btn;
- (IBAction)onViewPress:in_btn;

+ (BOOL)density;
+ (BOOL)velocity;
+ (BOOL)source;

+ (BOOL)viewDensity;
+ (BOOL)viewVelocity;

+ (float)brushSize;

+ (float)R;
+ (float)G;
+ (float)B;
@end
