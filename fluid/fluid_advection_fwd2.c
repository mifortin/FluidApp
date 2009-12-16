/*
 *  fluid_advection_fwd2.c
 *  FluidApp
 *
 *		This advection scheme attempts to accurately follow the characteristics
 *		of a given particle in order to improve advection.   The theory being
 *		that this will run extremely fast given it's deterministic.
 *
 */

#include "fluid_macros_2.h"
#include "fluid_cpu.h"
#include <math.h>
#include <stdio.h>

#ifdef __SSE3__
#include <xmmintrin.h>
#include <emmintrin.h>
#include <pmmintrin.h>
#include <tmmintrin.h>
//#undef __SSE3__
#endif


void fluid_advection_fwd_generate_repos(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct advection_stam_velocity *d = &mode->advection_stam_velocity;
	
	int w = fieldWidth(d->srcVelX);
	int h = fieldHeight(d->srcVelY);
	
	float *velX		= fieldData(d->srcVelX);
	float *velY		= fieldData(d->srcVelY);
	
	float *destX	= fieldData(d->dstVelX);
	float *destY	= fieldData(d->dstVelY);
	
	int sY = fieldStrideY(d->srcVelY);
	int sX = fieldStrideX(d->srcVelY);
	
	float timestep = -d->timestep / 9.0f;
	
	if (y==0)
	{}
	else if (y == h-1)
	{}
	else
	{
		float fx;
		float fy = (float)y;
		int x;
		for (x=0,fx=0; x<w; x++,fx++)
		{
			// Note (x+1 - (x-1))/2 = 2/2 = 1
			// Note (x-x)/2 = 0
			fluidFloatPointer(destX, x*sX + y*sY)[0] =
						fx
						+ timestep * fluidFloatPointer(velX,x*sX + y*sY)[0];
		}
		
		for (x=0; x<w; x++)
		{
			fluidFloatPointer(destY, x*sX + y*sY)[0] =
						fy
						+ timestep * fluidFloatPointer(velY, x*sX + y*sY)[0];
		}
	}
}



void fluid_advection_fwd_repos(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct advection_stam_velocity *d = &mode->advection_stam_velocity;
	
	int w = fieldWidth(d->srcVelX);
	int h = fieldHeight(d->srcVelY);
	
	float *velX		= fieldData(d->srcVelX);
	float *velY		= fieldData(d->srcVelY);
	
	float *destX	= fieldData(d->dstVelX);
	float *destY	= fieldData(d->dstVelY);
	
	int sY = fieldStrideY(d->srcVelY);
	int sX = fieldStrideX(d->srcVelY);
	
	float timestep = -d->timestep / 18.0f;
	
	if (y==0)
	{}
	else if (y == h-1)
	{}
	else
	{
		if (d->clamp)
		{
			float w2 = (float)w-2;
			float h2 = (float)h-2;
			int x;
			float fx;
			for (x=1,fx=1; x<w-1; x++,fx++)
			{
				float vx = fluidFloatPointer(velX, x*sX + y*sY)[0];
				float vy = fluidFloatPointer(velY, x*sX + y*sY)[0];
				
				float dLeft = fluidFloatPointer(destX, (x-1)*sX + y*sY)[0];
				float dRight = fluidFloatPointer(destX, (x+1)*sX + y*sY)[0];
				
				float dUp = fluidFloatPointer(destX, x*sX + (y-1)*sY)[0];
				float dDown = fluidFloatPointer(destX, x*sX + (y+1)*sY)[0];
				
				// Note (x+1 - (x-1))/2 = 2/2 = 1
				// Note (x-x)/2 = 0
				fluidFloatPointer(destX, x*sX + y*sY)[0] +=
						timestep * vx * (dRight - dLeft)
						+ timestep *vy * (dDown - dUp);
				
				fluidFloatPointer(destX, x*sX + y*sY)[0] = 
					fminf(fmaxf(fluidFloatPointer(destX, x*sX + y*sY)[0],0),w2);
				
				
				fluidFloatPointer(destX, x*sX + y*sY)[0] = 
					fminf(fmaxf(fluidFloatPointer(destX, x*sX + y*sY)[0],fx-9),fx+9);
			}
			
			float fy = (float)y;
			for (x=1; x<w-1; x++)
			{
				float vx = fluidFloatPointer(velX, x*sX + y*sY)[0];
				float vy = fluidFloatPointer(velY, x*sX + y*sY)[0];
				
				float dLeft = fluidFloatPointer(destY, (x-1)*sX + y*sY)[0];
				float dRight = fluidFloatPointer(destY, (x+1)*sX + y*sY)[0];
				
				float dUp = fluidFloatPointer(destY, x*sX + (y-1)*sY)[0];
				float dDown = fluidFloatPointer(destY, x*sX + (y+1)*sY)[0];
				
				// Note (x+1 - (x-1))/2 = 2/2 = 1
				// Note (x-x)/2 = 0
				fluidFloatPointer(destY, x*sX + y*sY)[0] +=
						timestep * vx * (dRight - dLeft)
						+ timestep *vy * (dDown - dUp);
				
				fluidFloatPointer(destY, x*sX + y*sY)[0] = 
					fminf(fmaxf(fluidFloatPointer(destY, x*sX + y*sY)[0],0),h2);
				
				fluidFloatPointer(destY, x*sX + y*sY)[0] = 
					fminf(fmaxf(fluidFloatPointer(destY, x*sX + y*sY)[0],fy-9),fy);
			}
		}
		else
		{
			int x;
			fluidFloatPointer(destX, y*sY)[0] = 0;
			for (x=1; x<w-1; x++)
			{
				float vx = fluidFloatPointer(velX, x*sX + y*sY)[0];
				float vy = fluidFloatPointer(velY, x*sX + y*sY)[0];
				
				float dLeft = fluidFloatPointer(destX, (x-1)*sX + y*sY)[0];
				float dRight = fluidFloatPointer(destX, (x+1)*sX + y*sY)[0];
				
				float dUp = fluidFloatPointer(destX, x*sX + (y-1)*sY)[0];
				float dDown = fluidFloatPointer(destX, x*sX + (y+1)*sY)[0];
				
				// Note (x+1 - (x-1))/2 = 2/2 = 1
				// Note (x-x)/2 = 0
				fluidFloatPointer(destX, x*sX + y*sY)[0] +=
						timestep * vx * (dRight - dLeft)
						+ timestep *vy * (dDown - dUp);
			}
			fluidFloatPointer(destX, x*sX + y*sY)[0] = (float)(w-1);
			
			for (x=1; x<w-1; x++)
			{
				float vx = fluidFloatPointer(velX, x*sX + y*sY)[0];
				float vy = fluidFloatPointer(velY, x*sX + y*sY)[0];
				
				float dLeft = fluidFloatPointer(destY, (x-1)*sX + y*sY)[0];
				float dRight = fluidFloatPointer(destY, (x+1)*sX + y*sY)[0];
				
				float dUp = fluidFloatPointer(destY, x*sX + (y-1)*sY)[0];
				float dDown = fluidFloatPointer(destY, x*sX + (y+1)*sY)[0];
				
				// Note (x+1 - (x-1))/2 = 2/2 = 1
				// Note (x-x)/2 = 0
				fluidFloatPointer(destY, x*sX + y*sY)[0] +=
					timestep * vx * (dRight - dLeft)
					+ timestep *vy * (dDown - dUp);
			}
		}

	}
}

