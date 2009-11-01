//
//  FluidApp_Delegate.m
//  FluidApp
//

#import "FluidApp_Delegate.h"
#import "field.h"
#import "FluidApp_Util.h"
#import "OpenGL/gl.h"
#include "memory.h"
#include "mpx.h"

#include <sys/types.h>
#include <sys/sysctl.h>
#include <unistd.h>


@implementation FluidApp_Delegate
	
	
	- (void)awakeFromNib
	{
		int mib[2] = {CTL_HW, HW_AVAILCPU};
		unsigned int i;
		size_t l=sizeof(i);
		sysctl(mib, 2, &i, &l, NULL, 0);
		//i = [[NSProcessInfo processInfo] processorCount];
	
		//l = sysconf(_SC_NPROCESSORS_CONF);
		if (i < 1) i = 1;
		printf("Initializing %i worker threads\n", i);
		
		x_init();
		mpInit(i);
	}
	
	- (void)dealloc
	{
		mpTerminate();
		[super dealloc];
	}

	
	
@end
