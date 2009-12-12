//
//  FluidAppSliderCell.m
//  FluidApp
//

#import "FluidAppSliderCell.h"


@implementation FluidAppSliderCell

- (void)drawBarInside:(NSRect)aRect flipped:(BOOL)flipped
{
	
	NSMutableParagraphStyle *ps = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
						[ps setAlignment:NSCenterTextAlignment];
	
	NSDictionary *fontBlack =
	[NSDictionary dictionaryWithObjects:
		 [NSArray arrayWithObjects:[NSColor blackColor],[NSFont fontWithName:@"Arial" size:10],ps,nil]
									forKeys:
		 [NSArray arrayWithObjects:NSForegroundColorAttributeName,NSFontAttributeName,NSParagraphStyleAttributeName,nil]];
	
	NSDictionary *fontWhite =
		[NSDictionary dictionaryWithObjects:
		 [NSArray arrayWithObjects:[NSColor whiteColor],[NSFont fontWithName:@"Arial" size:10],ps,nil]
									forKeys:
		 [NSArray arrayWithObjects:NSForegroundColorAttributeName,NSFontAttributeName,NSParagraphStyleAttributeName,nil]];
	
	NSString *szAmt = [NSString stringWithFormat:@"%0.2f", [self floatValue]];
	aRect.origin.x++;
	aRect.origin.y--;
	[szAmt drawInRect:aRect withAttributes:fontWhite];
	aRect.origin.x-=2;
	[szAmt drawInRect:aRect withAttributes:fontWhite];
	aRect.origin.x++;
	aRect.origin.y++;
	[szAmt drawInRect:aRect withAttributes:fontWhite];
	aRect.origin.y-=2;
	[szAmt drawInRect:aRect withAttributes:fontWhite];
	aRect.origin.y++;
	[szAmt drawInRect:aRect withAttributes:fontBlack];
}

- (void)drawKnob:(NSRect)knobRect
{
	[super drawKnob:knobRect];
	[[self controlView] setNeedsDisplay:YES];
	
}
@end
