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
	
	hosts[0] = @"localhost";
	hosts[1] = @"localhost";
	
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
	if (c == ib_status)	return status[iStat[r]];
	if (c == ib_host)	return hosts[r];
	if (c == ib_port)	return [NSString stringWithFormat:@"%i",iPorts[r]];
	
	return nil;
}

@end
