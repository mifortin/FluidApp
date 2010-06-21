//
//  FluidToolbar.m
//  FluidApp

#import "FluidToolbar.h"


@implementation FluidToolbar

- (void)awakeFromNib
{
	[headerContainer addSubview:basicTools];
	[headerContainer setNeedsDisplay:YES];
	visibleView = basicTools;
	
	[btnBasic makeLeftMost];
	[btnClient makeRightMost];
	
	allItems = [NSArray arrayWithObjects:btnBasic, btnDiffusion, btnQuality,
										btnClient, btnServer, btnTemperature,
										btnVisuals, nil];
	[allItems retain];
}


- (void)dealloc
{
	[allItems release];
	
	[super dealloc];
}


- (IBAction)onPressButton:in_btn
{
	int i;
	int c = [allItems count];
	for (i=0; i<c;i++)
	{
		id cur = [allItems objectAtIndex:i];
		
		if (cur == in_btn)
		{
			[cur setState:NSOnState];
			
			if (visibleView != nil)
				[visibleView removeFromSuperviewWithoutNeedingDisplay];
			
			if (in_btn == btnBasic)
				visibleView = basicTools;
			else if (in_btn == btnDiffusion)
				visibleView = diffusionTools;
			else if (in_btn == btnQuality)
				visibleView = qualityTools;
			else if (in_btn == btnClient)
				visibleView = clientTools;
			else if (in_btn == btnServer)
				visibleView = serverTools;
			else if (in_btn == btnTemperature)
				visibleView = temperature;
			else if (in_btn == btnVisuals)
				visibleView = visuals;
			else
				visibleView = nil;
			
			if (visibleView != nil)
				[headerContainer addSubview:visibleView];
		}
		else
			[cur setState:NSOffState];
	}
	[headerContainer setNeedsDisplay:YES];
	
	//NSLog(@"PRESSED");
}

@end
