//
//  FluidTransparentWindow.h
//  FluidApp
//

#import <Cocoa/Cocoa.h>


@interface FluidTransparentWindow : NSPanel
#ifndef MAC_10_4
<NSWindowDelegate>
#endif
{

}

- (void)displayForView:(NSView *)v;

- (void)displayForRect:(NSRect)r inView:(NSView*)v;

@end
