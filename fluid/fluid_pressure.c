/*
 *  fluid_pressure.c
 *  FluidApp
 */

#include "fluid_pvt.h"
#include "fluid_macros_2.h"

#include <math.h>
#include <stdio.h>

/** Most basic function used to handle pressure */
void fluid_genPressure(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct pressure *p = &mode->pressure;
	
	int w = fieldWidth(p->velX);
	int h = fieldHeight(p->velX);
	
	int sx = fieldStrideX(p->velX);
	int sy = fieldStrideY(p->velY);
	
	float *velX = fieldData(p->velX);
	float *velY = fieldData(p->velY);
	
	float *pressure = fieldData(p->pressure);
	
	if (y == 0)
	{
		int x;
		for (x=0; x<w; x++)
		{
			fluidFloatPointer(pressure,x*sx)[0] = fluidFloatPointer(pressure,x*sx + sy)[0];
		}
	}
	else if (y == h-1)
	{
		int x;
		for (x=0; x<w; x++)
		{
			fluidFloatPointer(pressure,x*sx + y*sy)[0] =
					fluidFloatPointer(pressure,x*sx + (y-1)*sy)[0];
		}
	}
	else
	{
		float lastPressureX = fluidFloatPointer(pressure,sx + y*sy)[0];
		float lastVelX = fluidFloatPointer(velX, y*sy)[0];
		
		float curPressureX = lastPressureX;
		float curVelX = fluidFloatPointer(velX, sx + y*sy)[0];
		
		fluidFloatPointer(pressure,y*sy)[0] = lastPressureX;
		
		int x;
		int curxy = sx + y*sy;
		for (x=1; x<w-1; x++)
		{
			float nextPressureX = fluidFloatPointer(pressure,curxy + sx)[0];
			float nextVelX = fluidFloatPointer(velX,curxy + sx)[0];
			
			fluidFloatPointer(pressure,curxy)[0] =
				(	  lastPressureX
				 	+ nextPressureX
				 	+ fluidFloatPointer(pressure,curxy - sy)[0]
					+ fluidFloatPointer(pressure,curxy + sy)[0]
				 - 		(  nextVelX
						 - lastVelX
						 + fluidFloatPointer(velY,curxy + sy)[0]
						 - fluidFloatPointer(velY,curxy - sy)[0])) / 4.0f;
			
			lastPressureX = curPressureX;
			curPressureX = nextPressureX;
			
			lastVelX = curVelX;
			curVelX = nextVelX;
			
			curxy += sx;
		}
		
		fluidFloatPointer(pressure,(w-1)*sx + y*sy)[0]
			= fluidFloatPointer(pressure,(w-2)*sx + y*sy)[0];
	}
}

/** Function to handle pressure while respecting densities
 	- To get zero pressure we clear out values where needed...*/
void fluid_genPressure_dens(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct pressure *p = &mode->pressure;
	
	int w = fieldWidth(p->velX);
	
	int sx = fieldStrideX(p->velX);
	int sy = fieldStrideY(p->velY);
	
	float *velX = fieldData(p->velX);
	float *velY = fieldData(p->velY);
	
	float *pressure = fieldData(p->pressure);
	
	float *density = fieldData(p->density);
	int dx = fieldStrideX(p->density);
	int dy = fieldStrideY(p->density);
	int dc = fieldComponents(p->density);
	
	float inv = 1.0f/(float)dc;
	
	int x;
	for (x=0; x<w; x++)
	{
		float avg = 0;
		int i;
		float *dp = fluidFloatPointer(density, dx*x + dy*y);
		for (i=0; i<dc; i++)
		{
			avg += dp[i];
		}
		
		avg *= inv;
		
		if (avg < 0.5f)	avg = 0.5f;
		
		fluidFloatPointer(pressure,x*sx + y*sy)[0] *= avg;
		fluidFloatPointer(velX, x*sx + y*sy)[0] *= avg;
		fluidFloatPointer(velY, x*sx + y*sy)[0] *= avg;
	}
}

/** Applies the results of the pressure */
void fluid_applyPressure(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct pressure *p = &mode->pressure;
	
	int w = fieldWidth(p->velX);
	int h = fieldHeight(p->velX);
	
	int sx = fieldStrideX(p->velX);
	int sy = fieldStrideY(p->velY);
	
	float *velX = fieldData(p->velX);
	float *velY = fieldData(p->velY);
	
	float *pressure = fieldData(p->pressure);
	
	if (y == 0)
	{
	}
	else if (y == h-1)
	{
	}
	else
	{		
		int x;
		for (x=1; x<w-1; x++)
		{
			*fluidFloatPointer(velX,x*sx + y*sy)
				-= *fluidFloatPointer(pressure,(x+1)*sx+y*sy)
					- *fluidFloatPointer(pressure,(x-1)*sx+y*sy);
			
			
			*fluidFloatPointer(velX,x*sx + y*sy)
				= fluidClamp(*fluidFloatPointer(velX,x*sx + y*sy),-9,9);
			
			
		}
		for (x=1; x<w-1; x++)
		{
			*fluidFloatPointer(velY,x*sx + y*sy)
				-= *fluidFloatPointer(pressure,x*sx+(y+1)*sy)
					- *fluidFloatPointer(pressure,x*sx+(y-1)*sy);
			
			*fluidFloatPointer(velY,x*sx + y*sy)
				= fluidClamp(*fluidFloatPointer(velY,x*sx + y*sy),-9,9);
		}
	}
}
