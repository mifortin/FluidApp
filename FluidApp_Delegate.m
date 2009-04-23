//
//  FluidApp_Delegate.m
//  FluidApp
//

#import "FluidApp_Delegate.h"
#import "FluidApp_Util.h"
#import "OpenGL/gl.h"
#include "memory.h"

@implementation FluidApp_Delegate
	
	
	- (void)awakeFromNib
	{
		allObjs = [[NSMutableArray alloc] initWithCapacity:10];
	}
	
	- (void)dealloc
	{
		[r_timer invalidate];
		Release(r_view);
		Release(r_timer);
		Release(allObjs);
		
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
		if (r_proto != NULL)
		{
			error *err;
			if (err = protocolLuaSend(r_proto, [[i_textView string] UTF8String]))
			{
				NSLog(@"Failed sending LUA script, %s", errorMsg(err));
				return;
			}
			[r_drawer open];
			
			return;
		}
		
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
		
		error *err;
		netClient *nc = netClientCreate([[i_netAddress stringValue] UTF8String],
									"2048", NETS_TCP, &err);
		if (nc == NULL)
		{
			NSLog(@"Failed connecting!");
			return;
		}
		
		r_proto = protocolCreate(nc, 1024*32, &err);
		if (r_proto == NULL)
		{
			NSLog(@"Failed creating protocol");
			return;
		}
		
		if (err = protocolLuaSend(r_proto, [[i_textView string] UTF8String]))
		{
			NSLog(@"Failed sending LUA script, %s", errorMsg(err));
			return;
		}
		
		protocolFloat*pf = protocolFloatCreate(r_proto, 10, NULL,
											   NULL, &err);
		
		
		//Add to the sideBar a custom view...
		int x;
		for (x=0; x<10; x++)
		{
			[NSBundle loadNibNamed:@"FluidNumber" owner:self];
			NSPoint fo = {32,0+x*20};
			NSView *v = [i_simpleNumber view];
			[v setFrameOrigin:fo];
			[v setHidden:NO];
			[i_sideBar addSubview:v];
			[i_sideBar setNeedsDisplay:YES];
			
			[i_simpleNumber bindToProtocol:pf atIndex:x];
			
			[v setAutoresizingMask:NSViewMaxYMargin];
			
			[allObjs addObject:i_simpleNumber];
			Release(i_simpleNumber);
		}
		
		//Now, open a drawer!!!
		NSSize cs = {160, [i_textWindow frame].size.height};
		r_drawer = [[NSDrawer alloc] initWithContentSize:cs preferredEdge:NSMaxXEdge];
		[r_drawer setParentWindow:i_textWindow];
		
		cs.height = 0;
		[r_drawer setMinContentSize:cs];
		cs.height = 4096;
		[r_drawer setMaxContentSize:cs];
		[r_drawer setContentView:i_sideBar];
		[i_sideBar setAutoresizingMask:NSViewHeightSizable|NSViewMaxYMargin];
		[r_drawer open];
	}
	
	
	- (void)onFrame:(NSTimer*)theTimer
	{
		[m_context makeCurrentContext];
		
		glClear(GL_COLOR_BUFFER_BIT);
		
		[m_context flushBuffer];
	}

	- (void)windowDidResize:(NSNotification *)notification
	{
		[self onFrame:r_timer];
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
