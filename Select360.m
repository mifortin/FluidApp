//
//  Select360.m
//  FluidApp
//

#import "Select360.h"

#include <math.h>

@implementation Select360

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
		m_value = (NSPoint){0,-1};
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
	NSRect rect = [self bounds];
	
	float cx = rect.size.width/2 + rect.origin.x;
	float cy = rect.size.height/2 + rect.origin.y;
	
	rect.size.width-=6;
	rect.size.height-=6;
	rect.origin.x+=3;
	rect.origin.y+=3;
	
	NSPoint c = {cx,cy};
	
	[[NSColor blackColor] set];
	
	NSBezierPath *p = [NSBezierPath bezierPathWithOvalInRect:rect];
	
	[p stroke];
	
	NSPoint d = {	m_value.x * rect.size.width/2 + cx,
					m_value.y * rect.size.height/2 + cy};
	
	[NSBezierPath strokeLineFromPoint:c toPoint:d];
	
	NSRect r2 = {cx-2,cy-2,4,4};
	[[NSBezierPath bezierPathWithOvalInRect:r2] fill];
}

- (void)mouseDown:(NSEvent*)e
{
	NSRect rect = [self bounds];
	
	float cx = rect.size.width/2 + rect.origin.x;
	float cy = rect.size.height/2 + rect.origin.y;
	
	NSPoint p = {cx,cy};
	
	NSPoint m = [self convertPoint:[e locationInWindow] fromView:nil];
	
	//printf("(%f %f) - (%f %f)\n",p.x, p.y, m.x, m.y);
	p.x = m.x - p.x;
	p.y = m.y - p.y;
	
	if (p.x*p.x + p.y*p.y > 3.0f)
	{
		float d = sqrtf(p.x*p.x + p.y*p.y);
		
		m_value.x = p.x/d;
		m_value.y = p.y/d;
	}
	
	[self setNeedsDisplay:YES];
}

- (void)mouseDragged:(NSEvent*)e
{
	[self mouseDown:e];
}


- (NSPoint)vector
{
	return m_value;
}

@end
