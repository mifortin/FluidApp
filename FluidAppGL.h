//
//  FluidAppGL.h
//  FluidApp
//

#import <Cocoa/Cocoa.h>

#include "mpx.h"
#include "fluid.h"


@interface FluidAppGL : NSObject
{
	fluid *r_fluid;
}

- (void)onFrame;

@end
