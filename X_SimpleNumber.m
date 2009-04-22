//
//  X_SimpleNumber.m
//  FluidApp
//

#import "X_SimpleNumber.h"


@implementation X_SimpleNumber

- (void)bindToProtocol:(protocolFloat*)in_proto atIndex:(int)in_index
{
	if (r_fl)
		x_free(r_fl);
	
	r_fl = in_proto;
	if (r_fl) x_retain(r_fl);
	
	m_refNumber = in_index;
}

- (IBAction)onTextChanged:(id)in_sender
{
	if (r_fl)
	{
		protocolFloatSend(r_fl, m_refNumber, [in_sender floatValue]);
	}
}

@end
