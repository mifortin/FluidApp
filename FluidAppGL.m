//
//  FluidAppGL.m
//  FluidApp
//

#import "FluidAppGL.h"
#include "memory.h"
#include <OpenGL/gl.h>

@implementation FluidAppGL


- init
{
	r_fluid = fluidCreate(512,512);
	return self;
}


- (void)dealloc
{
	x_free(r_fluid);
	[super dealloc];
}


- (void)onFrame
{
	fluidAdvance(r_fluid);

	glClear(GL_COLOR_BUFFER_BIT);
	
	glBegin(GL_QUADS);
	glColor4f(1,0,0,1);
	glVertex2f(-1,-1);
	glColor4f(0,1,0,1);
	glVertex2f(-1,1);
	glColor4f(0,0,1,1);
	glVertex2f(1,1);
	glVertex2f(1,-1);
	glEnd();
	
	glFlush();
}

@end
