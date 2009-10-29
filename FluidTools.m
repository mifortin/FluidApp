//
//  FluidTools.m
//  FluidApp
//

#import "FluidTools.h"

FluidTools *g_tools;

@implementation FluidTools

- (void)awakeFromNib
{
	NSLog(@"Found Toolbar!");
	g_tools = self;
	
	r_array = [[NSArray alloc] initWithObjects:btnDensity, btnVelocity, btnSource, nil];
	btnSelected = btnDensity;
	
	r_view = [[NSArray alloc] initWithObjects:viewDensity, viewVelocity, nil];
	viewSelected = viewDensity;
	
	[pnl_view setBecomesKeyOnlyIfNeeded:TRUE];
	[pnl_brush setBecomesKeyOnlyIfNeeded:TRUE];
	
	[[NSColorPanel sharedColorPanel] orderFront:nil];
	[[NSColorPanel sharedColorPanel] setColor:[NSColor colorWithDeviceRed:1.0f green:1.0f blue:1.0f alpha:1.0f]];
}

- (void)dealloc
{
	[r_array release];
	
	[super dealloc];
}

- (IBAction)onButtonPress:in_btn
{
	int x;
	for (x=0; x<[r_array count]; x++)
	{
		if ([r_array objectAtIndex:x] != in_btn)
			[[r_array objectAtIndex:x] setState:NSOffState];
		else
		{
			
			if ([r_array objectAtIndex:x] == btnDensity
				|| [r_array objectAtIndex:x] == btnSource)
				[[NSColorPanel sharedColorPanel] orderFront:nil];
			else
				[[NSColorPanel sharedColorPanel] orderOut:nil];
			
			btnSelected = (NSButton*)in_btn;
			[btnSelected setState:NSOnState];
		}
	}
}


- (IBAction)onViewPress:in_btn
{
	int x;
	for (x=0; x<[r_view count]; x++)
	{
		if ([r_view objectAtIndex:x] != in_btn)
			[[r_view objectAtIndex:x] setState:NSOffState];
		else
		{
			viewSelected = (NSButton*)in_btn;
			[viewSelected setState:NSOnState];
		}

	}
}


+ (BOOL)density		{	return g_tools->btnDensity == g_tools->btnSelected;		}
+ (BOOL)velocity	{	return g_tools->btnVelocity == g_tools->btnSelected;	}
+ (BOOL)source		{	return g_tools->btnSource == g_tools->btnSelected;		}

+ (BOOL)viewDensity { return g_tools->viewDensity == g_tools->viewSelected; }
+ (BOOL)viewVelocity { return g_tools->viewVelocity == g_tools->viewSelected; }

+ (float)brushSize	{ return [g_tools->brushSize floatValue]; }

+ (float)R		{ return [[[NSColorPanel sharedColorPanel] color] redComponent]; }
+ (float)G		{ return [[[NSColorPanel sharedColorPanel] color] greenComponent]; }
+ (float)B		{ return [[[NSColorPanel sharedColorPanel] color] blueComponent]; }
@end
