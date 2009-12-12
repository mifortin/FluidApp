//
//  FluidAppDocument.m
//  FluidApp
//

#import "FluidAppDocument.h"
#include <OpenGL/gl.h>
#include <sys/time.h>
#include "memory.h"

#ifdef MAC_10_4
#ifndef CGFloat
#define CGFloat float
#define NSInteger int
#endif
#endif


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
	[r_toolbar setVisible:YES];
	[r_toolbar setDisplayMode:NSToolbarDisplayModeIconOnly];
	[ib_window setToolbar:r_toolbar];
	
	[[ib_glView openGLContext] makeCurrentContext];
	[[ib_glView openGLContext] update];
	r_timer = [NSTimer scheduledTimerWithTimeInterval:1.0f/512.0f
											   target:self
											 selector:@selector(onFrame:)
											 userInfo:nil
											  repeats:YES];
	
	[ib_fpsView setTitle:@"Simulation" forIndex:0];
	[ib_fpsView setColor:[NSColor colorWithDeviceRed:1.0f green:0 blue:0 alpha:1] forIndex:0];
	
	m_prevTime = x_time();
	
	[ib_fpsView setTitle:@"Overall" forIndex:1];
	[ib_fpsView setColor:[NSColor colorWithDeviceRed:0.5f green:0 blue:0 alpha:1] forIndex:1];
	
	[ib_fpsView setTitle:@"Advection" forIndex:2];
	[ib_fpsView setColor:[NSColor colorWithDeviceRed:0 green:0.5f blue:0 alpha:1] forIndex:2];
	
	[ib_fpsView setTitle:@"Pressure" forIndex:3];
	[ib_fpsView setColor:[NSColor colorWithDeviceRed:0.5f green:0.5f blue:0 alpha:1] forIndex:3];
	
	[ib_fpsView setTitle:@"Viscosity" forIndex:4];
	[ib_fpsView setColor:[NSColor colorWithDeviceRed:0.5f green:0 blue:0.5f alpha:1] forIndex:4];
	
	[ib_fpsView setTitle:@"Vorticity" forIndex:5];
	[ib_fpsView setColor:[NSColor colorWithDeviceRed:0.0f green:0.5f blue:0.5f alpha:1] forIndex:5];
	
	[ib_fpsView setTitle:@"Scheduler" forIndex:6];
	[ib_fpsView setColor:[NSColor colorWithDeviceRed:0.3f green:0.3f blue:0.3f alpha:1] forIndex:6];
}

- (BOOL)windowShouldClose:(id)window
{
	[r_timer invalidate];
	return YES;
}

- (void)dealloc
{
	[[ib_glView openGLContext] makeCurrentContext];
	[[ib_glView openGLContext] update];
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




- (void)onPaint
{
	[[ib_glView openGLContext] makeCurrentContext];
	[[ib_glView openGLContext] update];
	glLoadIdentity();
	glViewport(0, 0, [ib_glView frame].size.width, [ib_glView frame].size.height);
	[ib_glView onPaint];
	[[ib_glView openGLContext] flushBuffer];
}


- (void)onFrame:(NSTimer*)theTimer
{
	double t = x_time();
	
	if ([ib_fpsView visible])
		[ib_glView enableTimers];
	else
		[ib_glView disableTimers];
	
	[ib_glView onFrame];
	[self onPaint];
	double t2 = x_time();
	[ib_fpsView setSample:(float)(t2-t)	forIndex:0];
	[ib_fpsView setSample:(float)(t2-m_prevTime)	forIndex:1];
	[ib_fpsView setSample:[ib_glView advectionTime] forIndex:2];
	[ib_fpsView setSample:[ib_glView pressureTime] forIndex:3];
	[ib_fpsView setSample:[ib_glView viscosityTime] forIndex:4];
	[ib_fpsView setSample:[ib_glView vorticityTime] forIndex:5];
	[ib_fpsView setSample:[ib_glView schedulerTime] forIndex:6];
	m_prevTime = t2;
	[ib_fpsView tick];
	//printf("dt: %f  fps: %f\n", (t2 - t), 1.0/(t2-t));
}

- (void)windowDidResize:(NSNotification *)notification
{
	[self onPaint];
}

- (void)windowDidUpdate:(NSNotification *)notification
{
	[self onPaint];
}

- (void)splitViewDidResizeSubviews:(NSNotification *)aNotification
{
	[self onPaint];
}

//Splits a subview!!!
- (CGFloat)splitView:(NSSplitView *)splitView
			constrainSplitPosition:(CGFloat)proposedPosition
		 	ofSubviewAt:(NSInteger)dividerIndex
{
	NSSize cSize =[splitView frame].size;
	if (proposedPosition > cSize.width - 150)
	{
		if (proposedPosition > cSize.width - 25)
			return [splitView frame].size.width;
		else
			return [splitView frame].size.width-150;
	}
	return proposedPosition;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Configuration code follows!
//
- (IBAction)onChangeViscosity:(id)value
{
	float v = [value floatValue];
	
	[ib_txt_viscosity setFloatValue:v];
	[ib_glView setViscosity:v];
}

- (IBAction)onChangeVorticity:(id)value
{
	float v = [value floatValue];
	
	[ib_txt_vorticity setFloatValue:v];
	[ib_glView setVorticity:v];
}

- (IBAction)onChangeTimestep:(id)value
{
	float v = [value floatValue];
	[ib_txt_timestep setFloatValue:v];
	[ib_glView setTimestep:v];
}

- (IBAction)onChangeFadeDensity:(id)value
{
	float v = [value floatValue];
	v = 1-(1-v)*(1-v);
	[ib_txt_fadeDensity setFloatValue:v];
	[ib_glView setFadeDensity:v];
}

- (IBAction)onChangeFadeVelocity:(id)value
{
	float v = [value floatValue];
	v = 1-(1-v)*(1-v);
	[ib_txt_fadeVelocity setFloatValue:v];
	[ib_glView setFadeVelocity:v];
}

- (IBAction)onChangeFreeSurface:(id)value
{
	if ([value state] == NSOnState)
		[ib_glView simpleFreeSurfaces];
	else
		[ib_glView noFreeSurfaces];
}

- (IBAction)onChangeVorticityQuality:(id)value
{
	if ([value state] == NSOnState)
		[ib_glView quickVorticity];
	else
		[ib_glView accurateVorticity];
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
