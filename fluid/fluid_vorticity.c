/*
 *  fluid_vorticity.c
 *  FluidApp
 */

#include "fluid_pvt.h"
#include "fluid_macros_2.h"
#include <math.h>

//First part, compute the divergence:
//
//	z-y, x-z, y-x
//	y z  z x  x y
//
void fluid_vorticity_curl(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct vorticity *v = &mode->vorticity;
	
	int h = fieldHeight(v->velX);
	int w = fieldWidth(v->velX);
	
	int sx = fieldStrideX(v->velX);
	int sy = fieldStrideY(v->velY);
	
	float *velX = fieldData(v->velX);
	float *velY = fieldData(v->velY);
	
	float *z = fieldData(v->z);
	
	int x;
	if (y==0)
	{
		x = 0;
		float dydx = (fluidFloatPointer(velY, (x+1)*sx)[0]
					  - fluidFloatPointer(velY, x*sx)[0]);
		float dxdy = (fluidFloatPointer(velX, sx + (1)*sy)[0]
					  - fluidFloatPointer(velX, sx)[0]);
		fluidFloatPointer(z, sx*x + sy*y)[0] = dydx - dxdy;
		
		for (x=1; x<w-1; x++)
		{
			dydx = (fluidFloatPointer(velY, (x+1)*sx)[0]
					- fluidFloatPointer(velY, (x-1)*sx)[0])/2;
			dxdy = (fluidFloatPointer(velX, sx + (1)*sy)[0]
					- fluidFloatPointer(velX, sx)[0]);
			fluidFloatPointer(z, sx*x + sy*y)[0] = dydx - dxdy;
		}
		
		dydx = (fluidFloatPointer(velY, x*sx)[0]
				- fluidFloatPointer(velY, (x-1)*sx)[0]);
		dxdy = (fluidFloatPointer(velX, sx + (1)*sy)[0]
				- fluidFloatPointer(velX, sx)[0]);
		fluidFloatPointer(z, sx*x + sy*y)[0] = dydx - dxdy;
	}
	else if (y == h-1)
	{
		x = 0;
		float dydx = (fluidFloatPointer(velY, (x+1)*sx + y*sy)[0]
					  - fluidFloatPointer(velY, x*sx + y*sy)[0]);
		float dxdy = (fluidFloatPointer(velX, sx + (y)*sy)[0]
					  - fluidFloatPointer(velX, sx + (y-1)*sy)[0]);
		fluidFloatPointer(z, sx*x + sy*y)[0] = dydx - dxdy;
		
		for (x=1; x<w-1; x++)
		{
			dydx = (fluidFloatPointer(velY, (x+1)*sx + y*sy)[0]
					- fluidFloatPointer(velY, (x-1)*sx + y*sy)[0])/2;
			dxdy = (fluidFloatPointer(velX, sx + (y)*sy)[0]
					- fluidFloatPointer(velX, sx + (y-1)*sy)[0]);
			fluidFloatPointer(z, sx*x + sy*y)[0] = dydx - dxdy;
		}
		
		dydx = (fluidFloatPointer(velY, x*sx + y*sy)[0]
				- fluidFloatPointer(velY, (x-1)*sx + y*sy)[0]);
		dxdy = (fluidFloatPointer(velX, sx + (y)*sy)[0]
				- fluidFloatPointer(velX, sx + (y-1)*sy)[0]);
		fluidFloatPointer(z, sx*x + sy*y)[0] = dydx - dxdy;
	}
	else
	{
		x = 0;
		float dydx = (fluidFloatPointer(velY, (x+1)*sx + y*sy)[0]
					  - fluidFloatPointer(velY, x*sx + y*sy)[0]);
		float dxdy = (fluidFloatPointer(velX, sx + (y+1)*sy)[0]
					  - fluidFloatPointer(velX, sx + (y-1)*sy)[0])/2;
		fluidFloatPointer(z, sx*x + sy*y)[0] = dydx - dxdy;
		
		for (x=1; x<w-1; x++)
		{
			dydx = (fluidFloatPointer(velY, (x+1)*sx + y*sy)[0]
						  - fluidFloatPointer(velY, (x-1)*sx + y*sy)[0])/2;
			dxdy = (fluidFloatPointer(velX, sx + (y+1)*sy)[0]
						  - fluidFloatPointer(velX, sx + (y-1)*sy)[0])/2;
			fluidFloatPointer(z, sx*x + sy*y)[0] = dydx - dxdy;
		}
		
		dydx = (fluidFloatPointer(velY, x*sx + y*sy)[0]
				- fluidFloatPointer(velY, (x-1)*sx + y*sy)[0]);
		dxdy = (fluidFloatPointer(velX, sx + (y+1)*sy)[0]
				- fluidFloatPointer(velX, sx + (y-1)*sy)[0])/2;
		fluidFloatPointer(z, sx*x + sy*y)[0] = dydx - dxdy;
	}
}


void fluid_vorticity_apply(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct vorticity *v = &mode->vorticity;
	
	int h = fieldHeight(v->velX);
	int w = fieldWidth(v->velX);
	
	int sx = fieldStrideX(v->velX);
	int sy = fieldStrideY(v->velY);
	
	float *velX = fieldData(v->velX);
	float *velY = fieldData(v->velY);
	
	float *z = fieldData(v->z);
	
	float e = v->e;
	
	int x;
	if (y==0)
	{
	}
	else if (y == h-1)
	{
	}
	else
	{
		float dzdx, dzdy, mag;
		
		for (x=1; x<w-1; x++)
		{
			dzdx = (fluidFloatPointer(z, (x+1)*sx + y*sy)[0]
					- fluidFloatPointer(z, (x-1)*sx + y*sy)[0])/2;
			dzdy = (fluidFloatPointer(z, sx + (y+1)*sy)[0]
					- fluidFloatPointer(z, sx + (y-1)*sy)[0])/2;
			float mg = dzdx*dzdx + dzdy*dzdy;
			if (mg > 0.001f)
			{
				mag = sqrtf(mg);
				dzdx= e*dzdx / mag;
				dzdy= e*dzdy / mag;
				
				fluidFloatPointer(velX, x*sx + sy*y)[0]
							+= dzdy *fluidFloatPointer(z, x*sx+ y*sy)[0];
				fluidFloatPointer(velY, x*sx + sy*y)[0]
				-= dzdx *fluidFloatPointer(z, x*sx+ y*sy)[0];
				
				*fluidFloatPointer(velX,x*sx + y*sy)
				= fluidClamp(*fluidFloatPointer(velX,x*sx + y*sy),-9,9);
				
				
				*fluidFloatPointer(velY,x*sx + y*sy)
				= fluidClamp(*fluidFloatPointer(velY,x*sx + y*sy),-9,9);
			}
		}
	}
}
