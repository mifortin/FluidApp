//
//  FluidTabTopView.m
//  FluidApp
//
//  Created by Michael Fortin on 09-12-05.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "FluidTabTopView.h"


@implementation FluidTabTopView

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
    }
    return self;
}

- (BOOL)mouseDownCanMoveWindow
{
	return YES;
}

- (void)drawRect:(NSRect)dirtyRect
{
    NSRect rect = [self bounds];
	rect.origin.x+=0.7f-2.5f;
	rect.origin.y-=rect.size.height;
	rect.size.height +=0.5f;
	
	NSBezierPath *bp = [NSBezierPath bezierPath];
	
	[bp setLineWidth:1.0f];
	
	NSPoint tmp = rect.origin;
	tmp.x += 2.5f;
	
	[bp moveToPoint:tmp];
	
	tmp = rect.origin;
	tmp.x += rect.size.width;
	
	tmp.x += 1.0f;
	NSPoint tmp2 = tmp;
	[bp lineToPoint:tmp];
	
	tmp.y += rect.size.height;
	tmp2 = tmp;
	tmp2.x -= 1.0f;
	[bp appendBezierPathWithArcFromPoint:tmp toPoint:tmp2 radius:5];
	
	tmp.x -= rect.size.width;
	[bp lineToPoint:tmp];
	
	[[NSColor whiteColor] set];
	[bp fill];
	[[NSColor darkGrayColor] set];
	[bp stroke];
}

@end
