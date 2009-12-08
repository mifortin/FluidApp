//
//  FluidAppServerController.h
//  FluidApp

#import <Cocoa/Cocoa.h>

#define FluidServerStatusGood		0
#define FluidServerStatusPending	1
#define FluidServerStatusFail		2

@interface FluidAppServerController : NSObject
{
	IBOutlet NSTableColumn *ib_data;
	IBOutlet NSTableColumn *ib_port;
	IBOutlet NSTableColumn *ib_image;
	IBOutlet NSTableColumn *ib_notes;
	
	IBOutlet NSWindow		*ib_window;
	
	NSImage *status[3];
	
	int iStat[2];		//Status for each row
	
	int iPorts[2];		//The ports (as selected by the user)
}

@end
