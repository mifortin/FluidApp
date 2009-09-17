//
//  FluidTools.h
//  FluidApp
//

#import <Cocoa/Cocoa.h>


@interface FluidTools : NSObject
{
	IBOutlet NSButton *btnDensity;
	IBOutlet NSButton *btnVelocity;
	
	NSArray *r_array;
	
	NSButton *btnSelected;
	
	
	IBOutlet NSButton *viewDensity;
	IBOutlet NSButton *viewVelocity;
	NSArray *r_view;
	NSButton *viewSelected;
}

- (IBAction)onButtonPress:in_btn;
- (IBAction)onViewPress:in_btn;

+ (BOOL)density;
+ (BOOL)velocity;

+ (BOOL)viewDensity;
+ (BOOL)viewVelocity;
@end
