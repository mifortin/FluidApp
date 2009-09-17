//
//  FluidAppGL.h
//  FluidApp
//

#import <Cocoa/Cocoa.h>
#include <OpenGL/gl.h>

#include "mpx.h"
#include "fluid.h"


@interface FluidAppGL : NSOpenGLView
{
	fluid *r_fluid;
	GLuint r_texture;
	
	NSPoint prevPt;
	
	float *work_buff;
}

- (void)onFrame;

@end
