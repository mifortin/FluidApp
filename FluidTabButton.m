//
//  FluidTabButton.m
//  FluidApp
//
//  Created by Michael Fortin on 09-12-05.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "FluidTabButton.h"


@implementation FluidTabButton

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        isLeftMost = NO;
		isRightMost = NO;
    }
    return self;
}


- (void)makeLeftMost
{
	isLeftMost = YES;
}

- (void)makeRightMost
{
	isRightMost = YES;
}

- (void)drawRect:(NSRect)dirtyRect
{
	NSRect rect = [self bounds];
	rect.origin.x+=0.7f;
	rect.origin.y+=0.7f;
	
	NSMutableParagraphStyle *ps = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
	[ps setAlignment:NSCenterTextAlignment];
	
	NSDictionary *fontBlack =
		[NSDictionary dictionaryWithObjects:
		 [NSArray arrayWithObjects:[NSColor blackColor],[NSFont fontWithName:@"Arial" size:10],ps,nil]
									forKeys:
		 [NSArray arrayWithObjects:NSForegroundColorAttributeName,NSFontAttributeName,NSParagraphStyleAttributeName,nil]];
	
	if ([self state] == NSOnState)
	{
		rect.size.height+=0.5f;
	}
	else
	{
		rect.size.height-=1.4f;
	}
	
	if (!isLeftMost)
	{
		rect.origin.x-=0.7f;
		rect.size.width+=0.7f;
	}
	
	if (!isRightMost)
	{
		rect.size.width+=0.7f;
	}
	
	rect.size.width -=2.5f;
	
	NSBezierPath *bp = [NSBezierPath bezierPath];
	
	[bp setLineWidth:2.23065f];
	
	NSPoint tmp = rect.origin;
	tmp.x += 2.5f;
	
	[bp moveToPoint:tmp];
	
	tmp = rect.origin;
	tmp.x += rect.size.width;
	
	tmp.x += 1.0f;
	NSPoint tmp2 = tmp;
	tmp2.y += 1.0f;
	[bp appendBezierPathWithArcFromPoint:tmp toPoint:tmp2 radius:5];
	
	tmp.y += rect.size.height;
	[bp lineToPoint:tmp];
	
	tmp.x -= rect.size.width;
	[bp lineToPoint:tmp];
	
	tmp.y -= rect.size.height;
	tmp2 = tmp;
	tmp2.x += 1.0f;
	[bp appendBezierPathWithArcFromPoint:tmp toPoint:tmp2 radius:5];
	
	if ([self state] == NSOnState)
		[[NSColor whiteColor] set];
	else
		[[NSColor lightGrayColor] set];
	[bp fill];
	[[NSColor darkGrayColor] set];
	[bp stroke];
	
	
	//[[self title] drawAtPoint:tmp withAttributes:fontBlack];
	rect = [self bounds];
	rect.origin.y += 3.0f;
	[[self title] drawInRect:rect withAttributes:fontBlack];
	
	[ps release];
}

@end
