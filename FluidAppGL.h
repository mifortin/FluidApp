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
	
	float m_dx;
	float m_dy;
	
	float *work_buff;
}

- (void)onFrame;

- (void)setViscosity:(float)in_v;
- (void)setVorticity:(float)in_v;
- (void)setTimestep:(float)in_v;

@end
