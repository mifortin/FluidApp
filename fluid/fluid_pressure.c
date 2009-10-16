/*
 *  fluid_pressure.c
 *  FluidApp
 */

#include "fluid_pvt.h"
#include "fluid_macros_2.h"

#include <math.h>
#include <stdio.h>

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
			*fluidFloatPointer(velX,x*sx + y*sy)
					= - *fluidFloatPointer(velX,x*sx + (y+1)*sy);
		}
	}
	else if (y == h-1)
	{
		int x;
		for (x=0; x<w; x++)
		{
			fluidFloatPointer(pressure,x*sx + y*sy)[0] =
					fluidFloatPointer(pressure,x*sx + (y-1)*sy)[0];
			
			*fluidFloatPointer(velX,x*sx + y*sy)
					= - *fluidFloatPointer(velX,x*sx + (y-1)*sy);
		}
	}
	else
	{
		fluidFloatPointer(pressure,y*sy)[0] = fluidFloatPointer(pressure,sx + y*sy)[0];
		
		int x;
		for (x=1; x<w-1; x++)
		{
			fluidFloatPointer(pressure,x*sx + y*sy)[0] =
				(	  fluidFloatPointer(pressure,(x-1)*sx + y*sy)[0]
				 	+ fluidFloatPointer(pressure,(x+1)*sx + y*sy)[0]
				 	+ fluidFloatPointer(pressure,x*sx + (y-1)*sy)[0]
					+ fluidFloatPointer(pressure,x*sx + (y+1)*sy)[0]
				 - 		(  fluidFloatPointer(velX,(x+1)*sx + y*sy)[0]
						 - fluidFloatPointer(velX,(x-1)*sx + y*sy)[0]
						 + fluidFloatPointer(velY,x*sx + (y+1)*sy)[0]
						 - fluidFloatPointer(velY,x*sx + (y-1)*sy)[0])) / 4.0f;
		}
		
		fluidFloatPointer(pressure,(w-1)*sx + y*sy)[0]
			= fluidFloatPointer(pressure,(w-2)*sx + y*sy)[0];
	}
}


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
			
			*fluidFloatPointer(velY,x*sx + y*sy)
				-= *fluidFloatPointer(pressure,x*sx+(y+1)*sy)
					- *fluidFloatPointer(pressure,x*sx+(y-1)*sy);
			
			*fluidFloatPointer(velX,x*sx + y*sy)
				= fluidClamp(*fluidFloatPointer(velX,x*sx + y*sy),-9,9);
			
			
			*fluidFloatPointer(velY,x*sx + y*sy)
				= fluidClamp(*fluidFloatPointer(velY,x*sx + y*sy),-9,9);
		}
	}
}
