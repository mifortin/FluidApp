//
//  FluidZip_Window.m
//  FluidApp

#import "FluidZip_Window.h"


@implementation FluidZip_Window

- (void)setImage:(NSImage *)in_image
{
	NSImageView *v = [[NSImageView alloc] init];
	
	if (r_img)	[r_img release];
	r_img = in_image;
	[r_img retain];
	
	[v setImage:in_image];
	[i_scrollView setDocumentView:v];
	[i_scrollView setNeedsDisplay:YES];
	
	NSPoint fo = {0,0};
	[v setFrameOrigin:fo];
	[v setFrameSize:[in_image size]];
	
	[v release];
}


- (NSImage*)image
{
	return r_img;
}


- (void)dealloc
{
	if (r_img) [r_img release];
	[super dealloc];
}


- (void)setTitle:(NSString*)in_title
{
	[i_window setTitle:in_title];
}


- (NSString*)title
{
	return [i_window title];
}

- (void) windowDidBecomeMain:(NSNotification *) notification
{
	if (m_del) [m_del onGotFocus:self];
}

- (void)setDelegate:(id<FluidZipWindowDelegate>)in_del
{
	m_del = in_del;
}

@end
