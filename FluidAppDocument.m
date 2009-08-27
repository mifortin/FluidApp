//
//  FluidAppDocument.m
//  FluidApp
//

#import "FluidAppDocument.h"
#include <OpenGL/gl.h>
#include <sys/time.h>

@implementation FluidAppDocument

- (void)windowControllerDidLoadNib:(NSWindowController *)windowController
{
	r_toolbarItems = [[NSArray alloc] initWithObjects:@"FluidTools", nil];
	
	r_toolbarItem = [[NSToolbarItem alloc] initWithItemIdentifier:@"FluidTools"];
	[r_toolbarItem setView:ib_toolbarView];
	
	r_toolbar = [[NSToolbar alloc] initWithIdentifier:@"FluidSimWindow"];
	[r_toolbar setDelegate:self];
	[r_toolbar setAllowsUserCustomization:NO];
	[r_toolbar setAutosavesConfiguration:NO];
	[r_toolbar setVisible:NO];
	[r_toolbar setDisplayMode:NSToolbarDisplayModeIconOnly];
	[ib_window setToolbar:r_toolbar];
	
	r_gl = [[FluidAppGL alloc] init];
	r_timer = [NSTimer scheduledTimerWithTimeInterval:1.0f/512.0f
											   target:self
											 selector:@selector(onFrame:)
											 userInfo:nil
											  repeats:YES];
}

- (BOOL)windowShouldClose:(id)window
{
	[r_timer invalidate];
	return YES;
}

- (void)dealloc
{
	[r_gl release];
	[r_toolbarItems release];
	[r_toolbar release];
	[super dealloc];
}

- (NSString *)windowNibName
{
    return @"FluidAppDocument";
}

- (NSData *)dataRepresentationOfType:(NSString *)type {
    // Implement to provide a persistent data representation of your document OR remove this and implement the file-wrapper or file path based save methods.
    return nil;
}

- (BOOL)loadDataRepresentation:(NSData *)data ofType:(NSString *)type {
    // Implement to load a persistent data representation of your document OR remove this and implement the file-wrapper or file path based load methods.
    return YES;
}


double timeFunc()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	
	return (double)t.tv_sec + ((double)t.tv_usec) / 1000000.0;
}


- (void)onFrame:(NSTimer*)theTimer
{
	double t = timeFunc();
	[[ib_glView openGLContext] makeCurrentContext];
	[[ib_glView openGLContext] update];
	glLoadIdentity();
	glViewport(0, 0, [ib_glView frame].size.width, [ib_glView frame].size.height);
	[r_gl onFrame];
	[[ib_glView openGLContext] flushBuffer];
	double t2 = timeFunc();
	printf("dt: %f  fps: %f\n", (t2 - t), 1.0/(t2-t));
}

- (void)windowDidResize:(NSNotification *)notification
{
	[self onFrame:nil];
}


////////////////////////////////////////////////////////////////////////////////
//
//	Toolbar code follows!
//
- (NSToolbarItem *)toolbar:(NSToolbar *)toolbar
			itemForItemIdentifier:(NSString *)itemIdentifier
			willBeInsertedIntoToolbar:(BOOL)flag
{
	if ([[r_toolbarItem itemIdentifier] compare:itemIdentifier] == NSOrderedSame)
		return r_toolbarItem;
	else
		return nil;
}

- (NSArray *)toolbarAllowedItemIdentifiers:(NSToolbar *)toolbar
{
	return r_toolbarItems;
}

- (NSArray *)toolbarDefaultItemIdentifiers:(NSToolbar *)toolbar
{
	return r_toolbarItems;
}

@end
