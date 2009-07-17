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

	}
	
	- (void)dealloc
	{		
		[super dealloc];
	}

	
	
@end
