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
	
	allItems = [NSArray arrayWithObjects:btnBasic, btnDiffusion, btnQuality, nil];
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
			else
				visibleView = nil;
			
			if (visibleView != nil)
				[headerContainer addSubview:visibleView];
		}
		else
			[cur setState:NSOffState];
	}
	[headerContainer setNeedsDisplay:YES];
	
	NSLog(@"PRESSED");
}

@end
