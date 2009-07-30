//
//  FluidAppGL.m
//  FluidApp
//

#import "FluidAppGL.h"

#include <OpenGL/gl.h>

@implementation FluidAppGL

- (void)onFrame
{
	glClear(GL_COLOR_BUFFER_BIT);
	
	glBegin(GL_QUADS);
	glVertex2f(-1,-1);
	glVertex2f(-1,1);
	glVertex2f(1,1);
	glVertex2f(1,-1);
	glEnd();
	
	glFlush();
}

@end
