//
//  FluidColorChooser.m
//  FluidApp
//

#import "FluidColorChooser.h"

static FluidColorChooser *si;

@implementation FluidColorChooser

- (void)awakeFromNib
{
	si = self;
}

+ (FluidColorChooser*)sharedInstance
{
	return si;
}


- (void)setColorRed:(float)in_r green:(float)in_g blue:(float)in_b
{
	[ib_red setFloatValue:in_r];
	[ib_green setFloatValue:in_g];
	[ib_blue setFloatValue:in_b];
	
	[ib_color setColor:[NSColor colorWithCalibratedRed:[ib_red floatValue]
								green:[ib_green floatValue]
								blue:[ib_blue floatValue]
								alpha:1.0f]];
}

- (IBAction)onChangeRed:in_sld
{
	[ib_color setColor:[NSColor colorWithCalibratedRed:[ib_red floatValue]
								green:[ib_green floatValue]
								blue:[ib_blue floatValue]
								alpha:1.0f]];
	
	[responder onChangeColorRed:[ib_red floatValue] green:[ib_green floatValue] blue:[ib_blue floatValue]];
}

- (IBAction)onChangeGreen:in_sld
{
	[ib_color setColor:[NSColor colorWithCalibratedRed:[ib_red floatValue]
								green:[ib_green floatValue]
								blue:[ib_blue floatValue]
								alpha:1.0f]];
	
	[responder onChangeColorRed:[ib_red floatValue] green:[ib_green floatValue] blue:[ib_blue floatValue]];
}

- (IBAction)onChangeBlue:in_sld
{
	[ib_color setColor:[NSColor colorWithCalibratedRed:[ib_red floatValue]
								green:[ib_green floatValue]
								blue:[ib_blue floatValue]
								alpha:1.0f]];
	
	[responder onChangeColorRed:[ib_red floatValue] green:[ib_green floatValue] blue:[ib_blue floatValue]];
}


- (void)setResponder:(id<FluidColorChooserResponder>)in_r
{
	responder = in_r;
}

@end
