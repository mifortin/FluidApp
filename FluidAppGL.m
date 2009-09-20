//
//  FluidAppGL.m
//  FluidApp
//

#import "FluidAppGL.h"
#include "memory.h"
#import "FluidTools.h"

#if defined( __SSE__ )
#include <xmmintrin.h>
#endif

@implementation FluidAppGL


- (void)awakeFromNib
{
	[[self openGLContext] makeCurrentContext];
	[[self openGLContext] update];
	r_fluid = fluidCreate(512,512);
	glGenTextures(1, &r_texture);
}


- (void)dealloc
{
	[[self openGLContext] makeCurrentContext];
	[[self openGLContext] update];
	glDeleteTextures(1, &r_texture);
	x_free(r_fluid);
	
	if (work_buff)	free(work_buff);
	
	[super dealloc];
}


- (void)mouseDown:(NSEvent*)theEvent
{
	NSPoint src = [[self window] convertScreenToBase:[NSEvent mouseLocation]];
	src = [self convertPoint:src fromView:nil];
	
	NSRect size = [self frame];
	
	src.x-=size.origin.x;
	src.y-=size.origin.y;
	
	src.x /= size.size.width;
	src.y /= size.size.height;
	
	prevPt = src;
	
	[self mouseDragged:theEvent];
}


- (void)mouseDragged:(NSEvent *)theEvent
{
	NSPoint src = [[self window] convertScreenToBase:[NSEvent mouseLocation]];
	src = [self convertPoint:src fromView:nil];
	
	NSRect size = [self frame];
	
	src.x-=size.origin.x;
	src.y-=size.origin.y;
	
	src.x /= size.size.width;
	src.y /= size.size.height;
	
	float dx = src.x - prevPt.x;
	float dy = src.y - prevPt.y;
	
	prevPt = src;
	
	field *dens = fluidDensity(r_fluid);
	int w = fieldWidth(dens);
	int h = fieldHeight(dens);
	float *d = fieldData(dens);
	
	src.x *= (float)w;
	src.y *= (float)h;
	
	if ([FluidTools density])
	{
		int x,y;
		for (y=(int)src.y-6; y<(int)src.y+6; y++)
		{
			if (y < 0 || y >= h)
				continue;
			
			for (x=(int)src.x-6; x<(int)src.x+6; x++)
			{
				if (x<0 || x>= w)
					continue;
				
				d[(x+y*w)*3+0] = 1.0f;
				d[(x+y*w)*3+1] = 1.0f;
				d[(x+y*w)*3+2] = 1.0f;
			}
		}
	}
	else if ([FluidTools velocity])
	{
		field *vx = fluidVelocityX(r_fluid);
		field *vy = fluidVelocityY(r_fluid);
		
		float *vdx = fieldData(vx);
		float *vdy = fieldData(vy);
		
		int x,y;
		for (y=(int)src.y-6; y<(int)src.y+6; y++)
		{
			if (y < 0 || y >= h)
				continue;
			
			for (x=(int)src.x-6; x<(int)src.x+6; x++)
			{
				if (x<0 || x>= w)
					continue;
				
				vdx[x+y*w] += dx*50;
				vdy[x+y*w] += dy*50;
			}
		}
	}
	
}

- (void)generateView
{
	if ([FluidTools viewDensity])
	{
		field *dens = fluidDensity(r_fluid);
		int w = fieldWidth(dens);
		int h = fieldHeight(dens);
		float *d = fieldData(dens);
		
		glBindTexture(GL_TEXTURE_2D, r_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0,
					 GL_RGB, GL_FLOAT, d);
	}
	else if ([FluidTools viewVelocity])
	{
		field *vx = fluidVelocityX(r_fluid);
		field *vy = fluidVelocityY(r_fluid);
		int w = fieldWidth(vx);
		int h = fieldHeight(vx);
		float *dx = fieldData(vx);
		float *dy = fieldData(vy);
		
		if (work_buff == NULL)
			work_buff = malloc(sizeof(float)*w*h*3);
		
		int x,y;
		for (y=0; y<h; y++)
		{
			for (x=0; x<w; x++)
			{
				work_buff[(x+y*w)*3+0] = (dx[x+y*w]+9.0f)/18.0f;
				work_buff[(x+y*w)*3+1] = (dy[x+y*w]+9.0f)/18.0f;
				work_buff[(x+y*w)*3+2] = 0;
			}
		}
		
		glBindTexture(GL_TEXTURE_2D, r_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0,
					 GL_RGB, GL_FLOAT, work_buff);
	}
}

- (void)onFrame
{
#if defined( __SSE__ )
	int oldMXCSR = _mm_getcsr(); //read the old MXCSR setting
	int newMXCSR = oldMXCSR | 0x00FFC0; // set DAZ and FZ bits
	_mm_setcsr( newMXCSR ); //write the new MXCSR setting to the MXCSR
	//printf("DENORMALS OFF!\n");
#endif
	
	x_try
		fluidAdvance(r_fluid);
		[self generateView];

	x_catch(e)
		printf("Something bad happened %s\n", errorMsg(e));
	x_finally
	
#if defined( __SSE__ )
	//restore old MXCSR settings to turn denormals back on if they were on
	_mm_setcsr( oldMXCSR );
#endif
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glEnable(GL_TEXTURE_2D);
	glClear(GL_COLOR_BUFFER_BIT);
	
	glColor4f(1,1,1,1);
	
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(-1,-1);
	glTexCoord2f(0, 1);
	glVertex2f(-1,1);
	glTexCoord2f(1, 1);
	glVertex2f(1,1);
	glTexCoord2f(1, 0);
	glVertex2f(1,-1);
	glEnd();
	
	glFlush();
}


- (void)setViscosity:(float)in_v
{
	fluidSetViscosity(r_fluid, in_v);
}

- (void)setVorticity:(float)in_v
{
	fluidSetVorticity(r_fluid, in_v);
}

@end
