/*
 *  fluid_viscosity.c
 *  FluidApp
 */


#include "fluid_pvt.h"
#include "fluid_macros_2.h"

void fluid_viscosity(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct viscosity *v = &mode->viscosity;
	
	int w = fieldWidth(v->velX);
	int h = fieldHeight(v->velX);
	
	int sx = fieldStrideX(v->velX);
	int sy = fieldStrideY(v->velY);
	
	float *velX = fieldData(v->velX);
	float *velY = fieldData(v->velY);
	
	float alpha = v->alpha;
	float beta = v->beta;
	
	if (y == 0)
	{
		int x;
		for (x=0; x<w; x++)
		{
			*fluidFloatPointer(velX,x*sx + y*sy)
							= - *fluidFloatPointer(velX,x*sx + (y+1)*sy);
			*fluidFloatPointer(velY,x*sx + y*sy)
							= - *fluidFloatPointer(velY,x*sx + (y+1)*sy);
		}
	}
	else if (y == h-1)
	{
		int x;
		for (x=0; x<w; x++)
		{			
			*fluidFloatPointer(velX,x*sx + y*sy)
							= - *fluidFloatPointer(velX,x*sx + (y-1)*sy);
			*fluidFloatPointer(velY,x*sx + y*sy)
							= - *fluidFloatPointer(velY,x*sx + (y-1)*sy);
		}
	}
	else
	{
		fluidFloatPointer(velX,y*sy)[0] = -fluidFloatPointer(velX,sx + y*sy)[0];
		fluidFloatPointer(velY,y*sy)[0] = -fluidFloatPointer(velY,sx + y*sy)[0];
		
		int x;
		for (x=1; x<w-1; x++)
		{
			fluidFloatPointer(velX,x*sx + y*sy)[0]
				= (fluidFloatPointer(velX,x*sx + y*sy)[0]*alpha
					+ fluidFloatPointer(velX,(x-1)*sx + y*sy)[0]
				    + fluidFloatPointer(velX,(x+1)*sx + y*sy)[0]
				    + fluidFloatPointer(velX,x*sx + (y-1)*sy)[0]
				   + fluidFloatPointer(velX,x*sx + (y+1)*sy)[0]) * beta;
			
			fluidFloatPointer(velY,x*sx + y*sy)[0]
				= (fluidFloatPointer(velY,x*sx + y*sy)[0]*alpha
				   + fluidFloatPointer(velY,(x-1)*sx + y*sy)[0]
				   + fluidFloatPointer(velY,(x+1)*sx + y*sy)[0]
				   + fluidFloatPointer(velY,x*sx + (y-1)*sy)[0]
				   + fluidFloatPointer(velY,x*sx + (y+1)*sy)[0]) * beta;
		}
		
		fluidFloatPointer(velX,(w-1)*sx + y*sy)[0]
						= -fluidFloatPointer(velX,(w-2)*sx + y*sy)[0];
		fluidFloatPointer(velY,(w-1)*sx + y*sy)[0]
						= -fluidFloatPointer(velY,(w-2)*sx + y*sy)[0];
	}
}

