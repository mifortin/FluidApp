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
		float curVelX = fluidFloatPointer(velX,sx + y*sy)[0];
		float curVelY = fluidFloatPointer(velY,sx + y*sy)[0];
		
		float nextVelX;
		float nextVelY;
		
		float pVelX = -curVelX;
		float pVelY = -curVelY;
		
		fluidFloatPointer(velX,y*sy)[0] = -curVelX;
		fluidFloatPointer(velY,y*sy)[0] = -curVelY;
		
		int x;
		for (x=1; x<w-1; x++)
		{
			nextVelX = fluidFloatPointer(velX,(x+1)*sx + y*sy)[0];
			nextVelY = fluidFloatPointer(velY,(x+1)*sx + y*sy)[0];
			
			fluidFloatPointer(velX,x*sx + y*sy)[0]
				= (fluidFloatPointer(velX,x*sx + y*sy)[0]*alpha
					+ pVelX
				    + nextVelX
				    + fluidFloatPointer(velX,x*sx + (y-1)*sy)[0]
				   + fluidFloatPointer(velX,x*sx + (y+1)*sy)[0]) * beta;
			
			fluidFloatPointer(velY,x*sx + y*sy)[0]
				= (fluidFloatPointer(velY,x*sx + y*sy)[0]*alpha
				   + pVelY
				   + nextVelY
				   + fluidFloatPointer(velY,x*sx + (y-1)*sy)[0]
				   + fluidFloatPointer(velY,x*sx + (y+1)*sy)[0]) * beta;
			
			pVelX = curVelX;
			pVelY = curVelY;
			
			curVelX = nextVelX;
			curVelY = nextVelY;
		}
		
		fluidFloatPointer(velX,(w-1)*sx + y*sy)[0]
						= -pVelX;
		fluidFloatPointer(velY,(w-1)*sx + y*sy)[0]
						= -pVelY;
	}
}

