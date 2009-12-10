//
//  FluidAppGL.h
//  FluidApp
//

#import <Cocoa/Cocoa.h>
#import "FluidAppServerController.h"
#import "FluidAppClientController.h"
#include <OpenGL/gl.h>

#include "mpx.h"
#include "fluid.h"


@interface FluidAppGL : NSOpenGLView <FluidAppServerDelegate,FluidAppClientDelegate>
{
	IBOutlet FluidAppServerController *ib_serverController;
	IBOutlet FluidAppClientController *ib_clientController;
	
	fieldServer *r_densityServer;
	fieldServer *r_velocityServer;
	fieldClient *r_densityClient;
	fieldClient *r_velocityClient;
	fluid *r_fluid;
	GLuint r_texture;
	
	NSPoint prevPt;
	
	float m_dx;
	float m_dy;
	
	float *work_buff;
	
	//The source (density added each frame regardless)
	float src_r;
	float src_g;
	float src_b;
	
	float src_rad;
	
	float src_x;
	float src_y;
	
	float src_dx;
	float src_dy;
}

- (void)onFrame;
- (void)onPaint;

- (void)setViscosity:(float)in_v;
- (void)setVorticity:(float)in_v;
- (void)setTimestep:(float)in_v;
- (void)setFadeVelocity:(float)in_v;
- (void)setFadeDensity:(float)in_v;

- (void)noFreeSurfaces;
- (void)simpleFreeSurfaces;

- (void)quickVorticity;
- (void)accurateVorticity;

- (void)enableTimers;
- (void)disableTimers;

- (float)advectionTime;
- (float)pressureTime;
- (float)viscosityTime;
- (float)vorticityTime;
- (float)schedulerTime;

@end
