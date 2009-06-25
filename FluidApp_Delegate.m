//
//  FluidApp_Delegate.m
//  FluidApp
//

#import "FluidApp_Delegate.h"
#import "field.h"
#import "FluidApp_Util.h"
#import "OpenGL/gl.h"
#include "memory.h"

error* myStringHandler(void *in_o, const char *in_szData)
{
	NSObject *o = (NSObject*)in_o;
	
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[o performSelectorOnMainThread:@selector(addToLog:)
						withObject:[NSString stringWithCString:in_szData
												  encoding:NSASCIIStringEncoding]
						waitUntilDone:YES];
	[pool release];
	return NULL;
}


error *myFloatHandler(protocolFloat *in_f, int in_id, void *in_o)
{
	NSObject *o = (NSObject*)in_o;
	
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[o performSelectorOnMainThread:@selector(updateFloat:)
					withObject:[NSNumber numberWithInt:in_id] waitUntilDone:NO];
	[pool release];
	
	return NULL;
}

error *myFieldHandler(field *in_f, int in_id, void *in_o)
{
	printf("GOT A MATRIX (%i)!!!\n\n", in_id);
	
	return NULL;
}


@implementation FluidApp_Delegate
	
	
	- (void)awakeFromNib
	{
		if (allObjs == nil)
		{
			allObjs = [[NSMutableArray alloc] initWithCapacity:10];
			[i_logView setEditable:NO];
		}
		
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
			
			
			[m_context makeCurrentContext];
			glGenTextures(1, &r_texture);
		}
	}
	
	- (void)dealloc
	{
		[r_timer invalidate];
		Release(r_view);
		Release(r_timer);
		Release(allObjs);
		
		[super dealloc];
	}


	- (void)addToLog:(NSString*)in_str
	{
		[i_logView setString:[[i_logView string] stringByAppendingString:in_str]];
		
	}

	- (void)updateFloat:(NSNumber*)in_index
	{
		int v = [in_index intValue];
		error *err;
		[[allObjs objectAtIndex:v] setFloat:protocolFloatReceive(r_pf, v, &err)];
	}
	

	- (IBAction)clearLog:(id)srcObj
	{
		[i_logView setString:@""];
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
		
		
		//Add to the sideBar a custom view...
		int x;
		netClient *nc = netClientCreate([[i_netAddress stringValue] UTF8String],
										"2048", NETS_TCP);
		if (nc == NULL)
		{
			NSLog(@"Failed connecting!");
			return;
		}
		
		error *err;
		r_proto = protocolCreate(nc, 1024*32, &err);
		if (r_proto == NULL)
		{
			NSLog(@"Failed creating protocol");
			return;
		}
		
		protocolStringCreate(r_proto, NULL, NULL, self, myStringHandler, &err);
		
		if (err = protocolLuaSend(r_proto, [[i_textView string] UTF8String]))
		{
			NSLog(@"Failed sending LUA script, %s", errorMsg(err));
			return;
		}
		
		r_pf = protocolFloatCreate(r_proto, 10, NULL,
											   NULL, &err);
		protocolFloatSetChangeHandler(r_pf, self, myFloatHandler);
		r_field = fieldCreate(r_proto, 512, 512, 3);
		
		for (x=0; x<10; x++)
		{
			[NSBundle loadNibNamed:@"FluidNumber" owner:self];
			NSPoint fo = {32,0+x*20};
			NSView *v = [i_simpleNumber view];
			[v setFrameOrigin:fo];
			[v setHidden:NO];
			[i_sideBar addSubview:v];
			[i_sideBar setNeedsDisplay:YES];
			
			[i_simpleNumber bindToProtocol:r_pf atIndex:x];
			
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
		
		//And a second drawer!
		cs.width = [i_textWindow frame].size.height;
		cs.height = 160;
		
		NSDrawer *drwer = [[NSDrawer alloc] initWithContentSize:cs
												  preferredEdge:NSMinYEdge];
		[drwer setParentWindow:i_textWindow];
		
		cs.width = 0;
		cs.height = 0;
		[drwer setMinContentSize:cs];
		cs.width =  4096;
		cs.height = 1024;
		[drwer setMaxContentSize:cs];
		
		[drwer setContentView:i_logBar];
		[drwer open];
		
		protocolSetReadyState(r_proto);
	}
	
	
	- (void)onFrame:(NSTimer*)theTimer
	{
		[m_context makeCurrentContext];
		
		glClear(GL_COLOR_BUFFER_BIT);
		
	//	error *err;
		if (r_field)
		{
			//float *d = fieldDataLock(r_field, &err);
			
			assert(glGetError() == GL_NO_ERROR);
			glEnable(GL_TEXTURE_2D);
			assert(glGetError() == GL_NO_ERROR);
			glBindTexture(GL_TEXTURE_2D, r_texture);
			assert(glGetError() == GL_NO_ERROR);
			//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 512, 512, 0, GL_RGB, GL_FLOAT, d);
			assert(glGetError() == GL_NO_ERROR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
			assert(glGetError() == GL_NO_ERROR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			
			assert(glGetError() == GL_NO_ERROR);
			
			glColor4f(1,1,1,1);
			glBindTexture(GL_TEXTURE_2D, r_texture);
			
			glBegin(GL_QUADS);
			glTexCoord2f(0,0);
			glVertex3f(-1, -1, 0);
			glTexCoord2f(1,0);
			glVertex3f(1, -1, 0);
			glTexCoord2f(1,1);
			glVertex3f(1, 1, 0);
			glTexCoord2f(0,1);
			glVertex3f(-1, 1, 0);
			glEnd();
			
			//fieldDataUnlock(r_field);
		}
		
		[m_context flushBuffer];
	}

	- (void)windowDidResize:(NSNotification *)notification
	{
		[self onFrame:r_timer];
	}


	- (void)windowDidBecomeMain:(NSNotification *)aNotification
	{
		
	}
	
	
	- (void)windowWillClose:(NSNotification *)aNotification
	{
		[NSApp terminate:self];
	}
	
	
@end
