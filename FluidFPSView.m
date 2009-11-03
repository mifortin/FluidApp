//
//  FluidFPSView.m
//  FluidApp
//
//  Created by Michael Fortin on 09-11-01.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "FluidFPSView.h"


@implementation FluidFPSView

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
    //Clear to white...
	[[NSColor whiteColor] set];
	[NSBezierPath fillRect:dirtyRect];
	
	//Draw grey lines for speed of simulation.
	//	1 line for each 0.1ms
	NSRect rect = [self bounds];
	float x;
	[[NSColor grayColor] set];
	NSDictionary *fontAttributes =
				[NSDictionary dictionaryWithObject:[NSColor grayColor]
								forKey:NSForegroundColorAttributeName];
	for (x=0; x<10; x++)
	{
		float y = (x+1)*rect.size.height/11.0f + rect.origin.y;
		NSPoint p1 = {rect.origin.x, y};
		NSPoint p2 = {rect.origin.x + rect.size.width, y};
		
		[NSBezierPath strokeLineFromPoint:p1 toPoint:p2];
		
		p1.x+=2.0f;
		p1.y+=2.1;
		[[NSString stringWithFormat:@"%2.2fms", x/30.0f]
		 drawAtPoint:p1 withAttributes:fontAttributes];
		
		p2.y+=2.1;
		p2.x-=56;
		[[NSString stringWithFormat:@"%2.2f fps", 1.0f/(x/30.0f)]
			 drawAtPoint:p2 withAttributes:fontAttributes];
	}
}

- (BOOL)isOpaque	{	return YES;		}

- (void)tick
{
	if ([self frame].size.width <= 0)	return;
	
	[self setNeedsDisplay:YES];
}

@end
