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
	
	
	- (void)awakeFromNib
	{
	}
	
	- (void)dealloc
	{
		[r_timer invalidate];
		Release(r_view);
		Release(r_timer);
		
		[super dealloc];
	}
	
	
	- (void)savePanelDidEnd:(NSSavePanel *)sheet returnCode:(int)returnCode
							contextInfo:(void  *)contextInfo
	{
		if (returnCode == NSOKButton)
		{
			NSLog(@"Saving to: %@", [sheet filename]);
			[i_textView writeRTFDToFile:[sheet filename] atomically:NO];
		}
	}
	
	
	- (IBAction)onSave:(id)in_src
	{
		NSSavePanel *s = [NSSavePanel savePanel];
		
		[s setExtensionHidden:NO];
		[s setAllowedFileTypes:[NSArray arrayWithObjects:@"rtfd",nil]];
		[s beginSheetForDirectory:nil
							file:nil
					modalForWindow:i_textWindow
					modalDelegate:self
					didEndSelector:@selector(savePanelDidEnd:returnCode:contextInfo:)
					contextInfo:nil];
	}
	
	
	- (void)openPanelDidEnd:(NSOpenPanel *)panel
				returnCode:(int)returnCode
				contextInfo:(void  *)contextInfo
	{
		if (returnCode == NSOKButton)
		{
			NSLog(@"Reading from: %@", [panel filename]);
			[i_textView readRTFDFromFile:[panel filename]];
		}
	}
	
	
	- (IBAction)onOpen:(id)in_src
	{
		NSOpenPanel *t = [NSOpenPanel openPanel];
		
		[t setExtensionHidden:NO];
		[t setAllowsMultipleSelection:NO];
		[t beginSheetForDirectory:nil file:nil
					types:[NSArray arrayWithObjects:@"rtfd",@"rtf",nil]
					modalForWindow:i_textWindow
					modalDelegate:self
					didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:)
					contextInfo:nil];
	}
	
	
	- (void)sheetDidEnd:(NSWindow *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo
	{
		[sheet orderOut:self];
	}
	
	
	- (IBAction)onConnect:(id)in_src
	{
		[NSApp beginSheet:i_hostSheet modalForWindow:i_textWindow
								modalDelegate:self
							didEndSelector:@selector(sheetDidEnd:returnCode:contextInfo:)
								contextInfo:nil];
	}
	
	
	- (IBAction)onCancelConnect:(id)in_src
	{
		[NSApp endSheet:i_hostSheet returnCode:NSCancelButton];
	}
	
	
	- (IBAction)onDoConnect:(id)in_src
	{
		[NSApp endSheet:i_hostSheet returnCode:NSOKButton];
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
	
	
@end
