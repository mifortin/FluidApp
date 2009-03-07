//
//  FluidApp_Delegate.m
//  FluidApp
//

#import "FluidApp_Delegate.h"
#import "FluidApp_Util.h"
#import "OpenGL/gl.h"

@implementation FluidApp_Delegate
	
	- init
	{
		if (![super init])
			return nil;
		
		return self;
	}
	
	
	- (void)onFrame:(NSTimer*)theTimer
	{
		[m_context makeCurrentContext];
		
		glClear(GL_COLOR_BUFFER_BIT);
		
		[m_context flushBuffer];
	}


	- (void)windowDidBecomeMain:(NSNotification *)aNotification
	{
		if (r_view == nil)
		{
			NSOpenGLPixelFormatAttribute attribs[] =
				{
					NSOpenGLPFAAlphaSize, 8,
					NSOpenGLPFADepthSize, 16,
					NSOpenGLPFADoubleBuffer, YES,
					0, 0
				};
				
			NSOpenGLPixelFormat *pf = [[NSOpenGLPixelFormat alloc]
										initWithAttributes:attribs];
			
			if (pf == nil)
			{
				NSLog(@"Failed creating pixel format");
				return;
			}
			
			r_view = [[NSOpenGLView alloc]
						initWithFrame:[i_window frame]
						pixelFormat: pf];
			Release(pf);
			
			if (r_view == nil)
			{
				NSLog(@"Unable to create GL View");
				return;
			}
			
			m_context = [r_view openGLContext];
			
			[i_window setContentView:r_view];
			
			r_timer = [NSTimer scheduledTimerWithTimeInterval:1.0f/120.0f
								target:self
								selector:@selector(onFrame:)
								userInfo: nil
								repeats:YES];
			[r_timer retain];
		}
	}
	
	
	- (void)windowWillClose:(NSNotification *)aNotification
	{
		[NSApp terminate:self];
	}
	
	
	- (void)dealloc
	{
		[r_timer invalidate];
		Release(r_view);
		Release(r_timer);
		
		[super dealloc];
	}
@end
