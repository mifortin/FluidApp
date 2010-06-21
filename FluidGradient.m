//
//  FluidGradient.m
//  FluidApp

#import "FluidGradient.h"

#ifdef MAC_10_4
#define CGFloat float
#endif

@implementation FluidGradient

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self)
	{
		int x;
		for (x=0; x<256; x++)
		{
			color[x].f[0] = (float)x/256.0f;
			color[x].f[1] = (float)x/256.0f;
			color[x].f[2] = (float)x/256.0f;
			color[x].f[3] = 1.0f;
			
			pos[x] = -1;
		}
		
		pos[0] = 0;
		pos[1] = 1;
		
		col[0].v = (x128f){0,0,0,1};
		col[1].v = (x128f){1,1,1,1};
		
		isClick = NO;
    }
    return self;
}


- (u128f*)gradient
{
	return color;
}


- (void)mouseDown:(NSEvent *)theEvent
{
	isClick = YES;
	NSRect r = [self bounds];
	NSPoint pt = [self convertPoint:[theEvent locationInWindow] fromView:nil];
	
	pt.x -= r.origin.x;
	pt.y -= r.origin.y;
	
	float left = pt.x - 5;
	float right = pt.x + 5;
	
	if (left < 0)	left = 0;
	
	left = left / r.size.width;
	right = right / r.size.width;


	int x;
	lastSelected = -1;
	for (x=0; x<256; x++)
	{
		if (pos[x] >= left && pos[x] <= right)
		{
			lastSelected = x;
			break;
		}
	}
	
	if (x == 256)
	{
		for (x=0; x<256; x++)
		{
			if (pos[x] < 0)
			{
				lastSelected = x;
				
				pos[x] = pt.x / r.size.width;
				int ipx = (pos[x]*255);
				col[x] = color[ipx];
				
				[self setNeedsDisplay:YES];
				
				break;
			}
		}
	}
}


- (void)rebuildGradient
{
	int cur = 0;
	for (;;)
	{
		int next = 0;
		if (next == cur)	next = 1;
		
		int x;
		for (x=0; x<256; x++)
		{
			if (x != cur && pos[x] > pos[cur] && (pos[x] < pos[next] || pos[next] < pos[cur]))
			{
				next = x;
			}
		}
		
		int i;
		float inc = 1.0f/((int)(pos[next]*256) - (int)(pos[cur]*256));
		float f = 0;
		for (i=pos[cur]*256; i<pos[next]*256; i++)
		{
			color[i].f[0] = col[cur].f[0] * (1-f) + col[next].f[0] * f;
			color[i].f[1] = col[cur].f[1] * (1-f) + col[next].f[1] * f;
			color[i].f[2] = col[cur].f[2] * (1-f) + col[next].f[2] * f;
			color[i].f[3] = col[cur].f[3] * (1-f) + col[next].f[3] * f;
			
			f+=inc;
		}
		
		cur = next;
		
		//NSLog(@"%i",cur);
		
		if (cur == 1)
			return;
	}
}


- (void)mouseDragged:(NSEvent *)theEvent
{
	isClick = NO;
	
	if (lastSelected > 1)
	{
		NSRect r = [self bounds];
		NSPoint pt = [self convertPoint:[theEvent locationInWindow] fromView:nil];
		
		pt.x -= r.origin.x;
		pt.y -= r.origin.y;
		
		float rat = pt.x / r.size.width;
		
		if (rat < 3 / r.size.width)
			rat = 3 / r.size.width;
		
		if (rat > (r.size.width-3)/r.size.width)
			rat = (r.size.width-3)/r.size.width;
		
		pos[lastSelected] = rat;
		
		if (pt.y > r.size.height+32 || pt.y < -32)
			pos[lastSelected] = -1;
		
		[self rebuildGradient];
		[self setNeedsDisplay:YES];
	}
}


- (void)mouseUp:(NSEvent *)theEvent
{
	if (isClick)
	{
		NSRect r = [self bounds];
		
		NSRect c = {r.origin.x + r.size.width*pos[lastSelected]-5,
						r.origin.y + r.size.height/2 - 9,
						0,10};
		
		[[FluidColorChooser sharedInstance] setResponder:self];
		[[FluidColorChooser sharedInstance]
				setColorRed:col[lastSelected].f[0]
						green:col[lastSelected].f[1]
						blue:col[lastSelected].f[2]];
		[[FluidColorChooser sharedInstance] displayForRect:c inView:self];
	}
}


-(void)onChangeColorRed:(float)in_r green:(float)in_g blue:(float)in_b
{
	if (lastSelected != -1)
	{
		col[lastSelected].f[0] = in_r;
		col[lastSelected].f[1] = in_g;
		col[lastSelected].f[2] = in_b;
		
		[self rebuildGradient];
		[self setNeedsDisplay:YES];
	}
}


- (void)drawRect:(NSRect)dirtyRect
{
	NSRect rect = [self bounds];
	
	//Use the top half for showing the color gradient...
	CGContextRef cg = [[NSGraphicsContext currentContext] graphicsPort];
	
	//Divide up the rect into small sections
	NSRect segment = rect;
	segment.size.height/=2;
	segment.size.width=1;
	
	segment.origin.y += segment.size.height;
	
	int x;
	for (x=0; x<rect.size.width; x++)
	{
		float apprx = (float)x * 255.0f / rect.size.width;
		int i = (int)apprx;
		
		CGContextSetRGBFillColor(cg, color[i].f[0], color[i].f[1], color[i].f[2], color[i].f[3]);
		CGContextFillRect(cg, *(CGRect*)&segment);
		segment.origin.x++;
	}
	
	CGContextSetRGBStrokeColor(cg, 0, 0, 0, 1);
	CGContextSetLineWidth(cg, 2.0f);
	
	
	float offY = rect.origin.y + rect.size.height/2-8;
	for (x=0; x<256; x++)
	{
		if (!(pos[x] < 0))
		{
			
			CGContextSetRGBFillColor(cg, col[x].f[0], col[x].f[1], col[x].f[2], col[x].f[3]);
			
			
			float offX = rect.origin.x + rect.size.width*pos[x];
			
			float left = -5 + offX;
			float right = 5+offX;
			
			if (pos[x] == 0)	left = offX;
			if (pos[x] == 1)	right = offX;
			
			CGContextBeginPath(cg);
			CGContextMoveToPoint(cg, offX, offY+10);
			CGContextAddLineToPoint(cg, left, offY+5);
			CGContextAddLineToPoint(cg, left, offY+0);
			CGContextAddLineToPoint(cg, right, offY+0);
			CGContextAddLineToPoint(cg, right, offY+5);
			CGContextClosePath(cg);
			CGContextFillPath(cg);
			
			CGContextBeginPath(cg);
			CGContextMoveToPoint(cg, offX, offY+10);
			CGContextAddLineToPoint(cg, left, offY+5);
			CGContextAddLineToPoint(cg, left, offY+0);
			CGContextAddLineToPoint(cg, right, offY+0);
			CGContextAddLineToPoint(cg, right, offY+5);
			CGContextClosePath(cg);
			CGContextStrokePath(cg);
		}
	}
}

@end
