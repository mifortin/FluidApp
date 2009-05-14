//
//  FluidZip_Window.h
//  FluidApp

#import <Cocoa/Cocoa.h>

@class FluidZip_Window;

@protocol FluidZipWindowDelegate
- (void)onGotFocus:(FluidZip_Window*)in_window;
@end

@interface FluidZip_Window : NSObject
{
	IBOutlet NSScrollView *i_scrollView;
	IBOutlet NSWindow *i_window;
	
	NSImage *r_img;
	
	id<FluidZipWindowDelegate> m_del;
}

- (void)setTitle:(NSString*)in_title;
- (NSString*)title;

- (void)setImage:(NSImage *)in_image;
- (NSImage*)image;

- (void)setDelegate:(id<FluidZipWindowDelegate>)in_del;
@end
