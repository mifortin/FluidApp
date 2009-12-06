//
//  FluidTabView.m
//  FluidApp
//

#import "FluidTabView.h"


@implementation FluidTabView

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
	
	NSRect rect = [self bounds];
	rect.origin.x+=0.7f;
	rect.origin.y+=1.5f;
	rect.size.width -=2.5f;
	rect.size.height -=0.5f;
	
	NSBezierPath *bp = [NSBezierPath bezierPath];
	
	[bp setLineWidth:1.0f];
	
	NSPoint tmp = rect.origin;
	tmp.x += 4.5f;
	
	[bp moveToPoint:tmp];
	
	tmp = rect.origin;
	tmp.x += rect.size.width;
	
	tmp.x += 1.0f;
	NSPoint tmp2 = tmp;
	tmp2.y += 1.0f;
	[bp appendBezierPathWithArcFromPoint:tmp toPoint:tmp2 radius:5];
	
	tmp.y += rect.size.height;
	tmp2 = tmp;
	tmp2.x -= 1.0f;
	[bp appendBezierPathWithArcFromPoint:tmp toPoint:tmp2 radius:5];
	
	tmp.x -= rect.size.width;
	[bp lineToPoint:tmp];
	
	tmp.y -= rect.size.height;
	tmp2 = tmp;
	tmp2.x += 1.0f;
	[bp appendBezierPathWithArcFromPoint:tmp toPoint:tmp2 radius:5];
	
	[[NSColor whiteColor] set];
	[bp fill];
	[[NSColor darkGrayColor] set];
	[bp stroke];
}

@end
