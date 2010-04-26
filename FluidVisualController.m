//
//  FluidVisualController.m
//  FluidApp
//

#import "FluidVisualController.h"
#import "FluidColorChooser.h"
#import "FluidAppDocument.h"


@implementation FluidVisualController

- (void)awakeFromNib
{
	[container addSubview:visualDefaultDensity];
	[container setNeedsDisplay:YES];
	visibleView = visualDefaultDensity;

	allItems = [NSArray arrayWithObjects:	visualDefaultDensity,
											visualTemperature,
											visualVelocity,
											nil];
	
	//printf("CHANGE COUNT %i\n",[allItems count]);
	[allItems retain];
}

- (void)dealloc
{
	[allItems release];
	
	[super dealloc];
}


- (IBAction)onChangeMode:in_mode
{
	int i = [mode indexOfSelectedItem];
	
	//printf("CHANGE MODE %i %i\n",i, [allItems count]);
	
	if (i >= [allItems count])
		return;
	
	NSView *nv = (NSView*)[allItems objectAtIndex:i];
	
	if (nv != visibleView)
	{
		[visibleView removeFromSuperviewWithoutNeedingDisplay];
		
		visibleView = nv;
		
		[container addSubview:nv];
		[container setNeedsDisplay:YES];
		
		[eh onSetVisual:i];
	}
}


- (u128f*)temperatureGradient
{
	return [temperature gradient];
}

@end
