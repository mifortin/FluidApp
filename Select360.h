//
//  Select360.h
//  FluidApp

#import <Cocoa/Cocoa.h>


@interface Select360 : NSView
{
	NSPoint m_value;
}

- (NSPoint)vector;

- (void)setVectorX:(float) x Y:(float)y;
@end
