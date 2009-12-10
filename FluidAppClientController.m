//
//  FluidAppClientController.m
//  FluidApp
//

#import "FluidAppClientController.h"

static NSString *g_ibData[] = {@"Velocity", @"Density"};

@implementation FluidAppClientController

- (void)awakeFromNib
{
	status[0] = [NSImage imageNamed:@"StatusGood.png"];
	status[1] = [NSImage imageNamed:@"StatusPending.png"];
	status[2] = [NSImage imageNamed:@"StatusFail.png"];
	
	iStat[0] = FluidClientStatusFail;
	iStat[1] = FluidClientStatusFail;
	
	iPorts[0] = 3535;
	iPorts[1] = 3636;
	
	hosts[0] = @"127.0.0.1";
	hosts[1] = @"127.0.0.1";
	
	[hosts[0] retain];
	[hosts[1] retain];
	
	int x;
	for (x=0; x<3; x++)	[status[x] retain];
}

- (void)dealloc
{
	int x;
	for (x=0; x<3; x++)	[status[x] release];
	
	[hosts[0] release];
	[hosts[1] release];
	
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
	if (c == ib_status)	return status[iStat[r]];
	if (c == ib_host)	return hosts[r];
	if (c == ib_port)	return [NSString stringWithFormat:@"%i",iPorts[r]];
	
	return nil;
}

- (void)tableView:(NSTableView *)v setObjectValue:(id)o forTableColumn:(NSTableColumn *)c row:(int)r
{
	if (c == ib_port)
	{
		int val = [o intValue];
		
		if (val <= 1024 ||  val > 65000)
			return;
		else if (iPorts[r] != val)
		{
			iPorts[r] = val;
			
			if (ib_delegate != nil)
				[ib_delegate onAlterClient:r host:hosts[r] port:iPorts[r]];
		}
	}
	else if (c == ib_host && [o isKindOfClass:[NSString class]])
	{
		if ([hosts[r] compare:o] != NSOrderedSame)
		{
			[hosts[r] release];
			hosts[r] = o;
			[hosts[r] retain];
			
			if (ib_delegate != nil)
				[ib_delegate onAlterClient:r host:hosts[r] port:iPorts[r]];
		}
	}
}

- (void)setStatus:(int)in_stat forClient:(int)in_client
{
	if (in_stat < 0 || in_stat >2)	return;
	if (in_client < 0 || in_client > 1) return;
	
	iStat[in_client] = in_stat;
	
	[[ib_port tableView] reloadData];
}

@end
