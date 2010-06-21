//
//  FluidDialogView.m
//  FluidApp
//

#import "FluidDialogView.h"


@implementation FluidDialogView

- (void)drawRect:(NSRect)dirtyRect
{
	
	NSRect rect = [self bounds];
	rect.origin.x+=0.7f;
	rect.origin.y+=0.7f;
	rect.size.width -=2.6f;
	rect.size.height -=2.6f + 5.0f;
	
	NSBezierPath *bp = [NSBezierPath bezierPath];
	
	[bp setLineWidth:2.0f];
	
	//Lower-left of shape
	NSPoint tmp = rect.origin;
	tmp.x += 1.0;
	[bp moveToPoint:tmp];
	
	//Lower-right of shape
	tmp = rect.origin;
	tmp.x += rect.size.width;
	tmp.x += 1.0f;
	NSPoint tmp2 = tmp;
	tmp2.y += 1.0f;
	[bp appendBezierPathWithArcFromPoint:tmp toPoint:tmp2 radius:5];
	
	//Top-right of shape
	tmp = rect.origin;
	tmp.x += rect.size.width;
	tmp.x += 1.0f;
	tmp.y += rect.size.height;
	tmp2 = tmp;
	tmp2.y += 1.0f;
	tmp2.x -= 1.0f;
	[bp appendBezierPathWithArcFromPoint:tmp toPoint:tmp2 radius:5];
	
	//A few pixels before middle...
	tmp = rect.origin;
	tmp.x += rect.size.width/2.0f + 5.0f;
	tmp.y += rect.size.height;
	[bp lineToPoint:tmp];
	
	//A few pixels at center
	tmp = rect.origin;
	tmp.x += rect.size.width/2.0f;
	tmp.y += rect.size.height + 5.0f;
	[bp lineToPoint:tmp];
	
	//A few pixels after center
	tmp = rect.origin;
	tmp.x += rect.size.width/2.0f - 5.0f;
	tmp.y += rect.size.height;
	[bp lineToPoint:tmp];
	
	//Top-left of shape
	tmp = rect.origin;
	tmp.y += rect.size.height;
	tmp.y += 1.0f;
	tmp2 = tmp;
	tmp2.y -= 1.0f;
	[bp appendBezierPathWithArcFromPoint:tmp toPoint:tmp2 radius:5];
	
	//Bottom-left of shape
	tmp = rect.origin;
	tmp2 = tmp;
	tmp2.x += 1.0f;
	[bp appendBezierPathWithArcFromPoint:tmp toPoint:tmp2 radius:5];
	
	[[NSColor colorWithDeviceRed:0.0f green:0.0f blue:0.0f alpha:0.75f] set];
	[bp fill];
	[[NSColor whiteColor] set];
	[bp stroke];
}

@end
