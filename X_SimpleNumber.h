//
//  X_SimpleNumber.h
//  FluidApp
//

#import <Cocoa/Cocoa.h>
#include "protocol.h"
#include "memory.h"

@interface X_SimpleNumber : NSView
{
	//The number that we reference...
	int m_refNumber;
	
	//Protocol used to send/receive data...
	protocolFloat *r_fl;
}

- (void)bindToProtocol:(protocolFloat*)in_proto atIndex:(int)in_index;

- (IBAction)onTextChanged:(id)in_sender;

@end
