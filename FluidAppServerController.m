//
//  FluidAppServerController.m
//  FluidApp
//

#import "FluidAppServerController.h"

@implementation FluidAppServerController

static NSString *g_ibData[] = {@"Velocity", @"Density"};


- (void)awakeFromNib
{
	status[0] = [NSImage imageNamed:@"StatusGood.png"];
	status[1] = [NSImage imageNamed:@"StatusPending.png"];
	status[2] = [NSImage imageNamed:@"StatusFail.png"];
	
	iStat[0] = FluidServerStatusFail;
	iStat[1] = FluidServerStatusFail;
	
	iPorts[0] = 2525;
	iPorts[1] = 2626;
	
	blend[0] = 0.99f;
	blend[1] = 0.99f;
	
	int x;
	for (x=0; x<3; x++)	[status[x] retain];
}

- (void)dealloc
{
	int x;
	for (x=0; x<3; x++)	[status[x] release];
	
	[super dealloc];
}

- (int)numberOfRowsInTableView:(NSTableView*)v
{
	return 2;
}

- (id)tableView:(NSTableView *)v objectValueForTableColumn:(NSTableColumn *)c
					row:(int)r
{
	if (c == ib_data)	return g_ibData[r];
	if (c == ib_image)	return status[iStat[r]];
	if (c == ib_port)	return [NSString stringWithFormat:@"%i",iPorts[r]];
	if (c == ib_blend)	return [NSNumber numberWithFloat:blend[r]];
	
	return nil;
}

- (BOOL)tableView:(NSTableView *)aTableView shouldSelectRow:(int)rowIndex
{
	NSCell *myCell = [ib_port dataCellForRow:rowIndex];
	[[myCell controlView] setFocusRingType:NSFocusRingTypeNone];
	
	return YES;
}

- (void)tableView:(NSTableView *)v setObjectValue:(id)o forTableColumn:(NSTableColumn *)c row:(int)r
{
	if (c == ib_port)
	{
		int val = [o intValue];
		
		if (val <= 1024 || val > 65000)		//Block root ports
			return;
		else if (iPorts[r] != val)
		{
			iPorts[r] = val;
			
			if (ib_delegate != nil)
				[ib_delegate onServerController:self forServer:r changePort:val];
		}
	}
	else if (c == ib_blend)
	{
		blend[r] = [o floatValue];
	}
}


- (void)setStatus:(int)in_status forServer:(int)in_server
{
	if (in_status < 0 || in_status > 2)
		return;
	
	if (in_server < 0 || in_server > 1)
		return;
	
	iStat[in_server] = in_status;
	
	[[ib_port tableView] reloadData];
}

@end
