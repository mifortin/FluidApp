//
//  FluidAppServerController.h
//  FluidApp

#import <Cocoa/Cocoa.h>

#define FluidServerStatusGood		0
#define FluidServerStatusPending	1
#define FluidServerStatusFail		2


@protocol FluidAppServerDelegate;


@interface FluidAppServerController : NSObject
{
	IBOutlet NSTableColumn *ib_data;
	IBOutlet NSTableColumn *ib_port;
	IBOutlet NSTableColumn *ib_image;
	IBOutlet NSTableColumn *ib_blend;
	
	IBOutlet NSWindow		*ib_window;
	
	IBOutlet id<FluidAppServerDelegate> ib_delegate;
	
	NSImage *status[3];
	
	int iStat[2];		//Status for each row
	
	int iPorts[2];		//The ports (as selected by the user)
	
	float blend[2];		//Means of controlling the blending...
}

- (void)setStatus:(int)in_status forServer:(int)in_server;

- (float)blendForServer:(int)in_server;

- (BOOL)serverConnected:(int)in_server;

@end


@protocol FluidAppServerDelegate

- (void)onServerController:(FluidAppServerController*)in_fasc
				 forServer:(int)in_serv
				changePort:(int)in_port;

@end
