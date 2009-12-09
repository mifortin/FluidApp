//
//  FluidAppClientController.h
//  FluidApp
//

#import <Cocoa/Cocoa.h>


#define FluidClientStatusGood		0
#define FluidClientStatusPending	1
#define FluidClientStatusFail		2

@interface FluidAppClientController : NSObject
{
	//The rows / Columans....
	IBOutlet NSTableColumn *ib_data;
	IBOutlet NSTableColumn *ib_host;
	IBOutlet NSTableColumn *ib_port;
	IBOutlet NSTableColumn *ib_status;
	
	//Images for status
	NSImage *status[3];
	
	//Data
	int iStat[2];
	int iPorts[2];
	NSString *hosts[2];
}

@end
