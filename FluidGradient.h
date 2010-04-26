//
//  FluidGradient.h
//  FluidApp

#import <Cocoa/Cocoa.h>
#include "x_simd.h"
#import "FluidColorChooser.h"

@interface FluidGradient : NSView <FluidColorChooserResponder>
{
	u128f color[256];
	
	float pos[256];		//Position of the colors
	u128f col[256];		//Actual colors
	
	BOOL isClick;
	
	int lastSelected;
}

- (u128f*)gradient;

@end
