//
//  X_SimpleNumber.h
//  FluidApp
//

#import <Cocoa/Cocoa.h>
#include "protocol.h"
#include "memory.h"

@interface X_SimpleNumber : NSObject
{
	//The number that we reference...
	int m_refNumber;
	
	//Protocol used to send/receive data...
	protocolFloat *r_fl;
	
	IBOutlet NSView *i_subview;
	
	IBOutlet NSTextField *i_txtField;
}

- (void)bindToProtocol:(protocolFloat*)in_proto atIndex:(int)in_index;

- (IBAction)onTextChanged:(id)in_sender;

- (void)setFloat:(float)in_value;

- (NSView*)view;

@end
