//
//  FluidZip_Delegate.h
//  FluidApp
//

#import <Cocoa/Cocoa.h>
#import "FluidZip_Window.h"

@interface FluidZip_Delegate : NSObject <FluidZipWindowDelegate>
{
	NSMutableArray *r_windows;
	FluidZip_Window *m_focus;
	
	IBOutlet FluidZip_Window *i_window;
}

- (IBAction)onMenuFileOpen:(id)in_src;
- (IBAction)onMenuFormatImageCompress:(id)in_src;

@end
