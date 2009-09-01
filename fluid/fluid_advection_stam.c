/*
 *  fluid_advection_stam.c
 *  FluidApp
 */

#include "fluid_pvt.h"
#include "fluid_macros_2.h"

void fluid_advection_stam_velocity(fluid *in_f, int y)
{
	int x;										//Simple looping var
	int w = fieldWidth(in_f->r_velocityX);		//Width of the data
	int h = fieldHeight(in_f->r_velocityX);
	
	float *velX		= fieldData(in_f->r_velocityX);
	float *velY		= fieldData(in_f->r_velocityY);
	
	float *velDestX	= fieldData(in_f->r_tmpVelX);
	float *velDestY	= fieldData(in_f->r_tmpVelY);
	
	int sX = fieldStrideX(in_f->r_velocityX);
	int sY = fieldStrideY(in_f->r_velocityY);
	
	//Extract the data from the object

	//Loop over the data and do the desired computation
	for (x=0; x<w; x++)
	{
		float *fDataX = fluidFloatPointer(velX, x*sX + y*sY);
		float *fDataY = fluidFloatPointer(velY, x*sX + y*sY);
		
		//Find the cell back in time	(keep a -10,10 radius)
		float backX = -fluidClamp(TIMESTEP * fDataX[0],-9,9) + (float)x;
		float backY = -fluidClamp(TIMESTEP * fDataY[0],-9,9) + (float)y;
		
		int nBackX = (int)backX;
		int nBackY = (int)backY;
		
		float scaleX = backX - (float)nBackX;
		float scaleY = backY - (float)nBackY;
		
		//Clamp as it's easier to parallelize given the scheduler
		nBackX = fluidClamp(nBackX, 0,w-2);
		nBackY = fluidClamp(nBackY, 0,h-2);
		
		//That was easy... now advect the velocity...
		float *fVelDestX = fluidFloatPointer(velDestX, x*sX + y*sY);
		float *fVelDestY = fluidFloatPointer(velDestY, x*sX + y*sY);
		
		int offBackX = nBackX * sX;
		int offBackY = nBackY * sY;
		int offX2 = (nBackX+1)*sX;
		int offY2 = (nBackY+1)*sY;
		
		float *bZZ_x = fluidFloatPointer(velX, offBackX + offBackY);
		float *bOZ_x = fluidFloatPointer(velX, offX2 + offBackY);
		float *bZO_x = fluidFloatPointer(velX, offBackX + offY2);
		float *bOO_x = fluidFloatPointer(velX, offX2 + offY2);
		
		float *bZZ_y = fluidFloatPointer(velY, offBackX + offBackY);
		float *bOZ_y = fluidFloatPointer(velY, offX2 + offBackY);
		float *bZO_y = fluidFloatPointer(velY, offBackX + offY2);
		float *bOO_y = fluidFloatPointer(velY, offX2 + offY2);
		
		fVelDestX[0] = fluidLinearInterpolation(scaleX, scaleY,
										  bZZ_x[0], bOZ_x[0], bZO_x[0], bOO_x[0]);
		fVelDestY[0] = fluidLinearInterpolation(scaleX, scaleY,
										  bZZ_y[0], bOZ_y[0], bZO_y[0], bOO_y[0]);
	}
}
