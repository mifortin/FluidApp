//
//  FluidTransparentWindow.m
//  FluidApp
//

#import "FluidTransparentWindow.h"

#ifdef MAC_10_4
#define NSUInteger int
#endif

@implementation FluidTransparentWindow

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)windowStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation
{
	[super initWithContentRect:contentRect
					styleMask: NSBorderlessWindowMask
					backing:bufferingType
					defer:deferCreation];
	
	[self setOpaque:NO];
	[self setHasShadow:YES];
	[self setBackgroundColor:[NSColor clearColor]];
	[self setLevel:NSFloatingWindowLevel];
	[self setBecomesKeyOnlyIfNeeded:NO];
	[self setDelegate:self];
	
	return self;
}

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)windowStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation screen:(NSScreen *)screen
{
	[super initWithContentRect:contentRect
					styleMask: NSBorderlessWindowMask
					backing:bufferingType
					defer:deferCreation
					screen:screen];
	
	[self setOpaque:NO];
	[self setHasShadow:YES];
	[self setBackgroundColor:[NSColor clearColor]];
	[self setLevel:NSFloatingWindowLevel];
	[self setBecomesKeyOnlyIfNeeded:NO];
	[self setDelegate:self];
	
	return self;
}

- (BOOL)canBecomeKeyWindow
{
	return YES;
}

- (BOOL)canBecomeMainWindow
{
	return NO;
}

//- (void)becomeKeyWindow
//{
//	[super becomeKeyWindow];
//	if ([self isKeyWindow])
//		NSLog(@"Already Was KEY!!!");
//	else
//		NSLog(@"First-time KEY!!!");
//	
//}

- (void)windowDidBecomeKey:(NSNotification *)notification
{
	if ([self isKeyWindow])
		NSLog(@"Already Was KEY!!!");
	else
		NSLog(@"First-time KEY!!!");
}

- (void)windowDidResignKey:(NSNotification *)notification
{
	NSLog(@"Window no longer KEY!!!");
	[self orderOut:self];
}

//- (void)resignKeyWindow
//{
//	NSLog(@"Window no longer KEY!!!");
//	[self orderOut:self];
//}


- (void)displayForRect:(NSRect)r inView:(NSView*)v
{
	NSWindow *w = [v window];
	if (w)
	{
		NSPoint ct = r.origin;
		ct.x += r.size.width/2;
		ct.y += r.size.height/2;
		NSPoint scCoord = [v convertPoint:ct toView:nil];
		scCoord = [w convertBaseToScreen:scCoord];
		scCoord.x -= [w frame].size.width/4;
		[self setFrameTopLeftPoint:scCoord];
		[self makeKeyAndOrderFront:self];
	}
}

- (void)displayForView:(NSView *)v
{
	
	NSWindow *w = [v window];
	if (w)
	{
		NSPoint ct = [v bounds].origin;
		ct.x += [v bounds].size.width/2;
		ct.y += [v bounds].size.height/2;
		NSPoint scCoord = [v convertPoint:ct toView:nil];
		scCoord = [w convertBaseToScreen:scCoord];
		scCoord.x -= [w frame].size.width/4;
		[self setFrameTopLeftPoint:scCoord];
		[self makeKeyAndOrderFront:self];
		
		//NSLog(@"Moved to %f %f", scCoord.x, scCoord.y);
	}
}



@end
