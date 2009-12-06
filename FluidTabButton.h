//
//  FluidTabButton.h
//  FluidApp

#import <Cocoa/Cocoa.h>


@interface FluidTabButton : NSButton
{
	
	BOOL isLeftMost;
	BOOL isRightMost;
}

- (void)makeLeftMost;
- (void)makeRightMost;

@end
