/*
 *  fluid_advection_stam.c
 *  FluidApp
 */

#include "fluid_pvt.h"
#include "fluid_macros_2.h"

//Basic advection of only the velocity...
void fluid_advection_stam_velocity(fluid *in_f, int y, pvt_fluidMode *mode)
{
	int x;
	int w = fieldWidth(mode->advection_stam_velocity.srcVelX);
	int h = fieldHeight(mode->advection_stam_velocity.srcVelY);
	
	float *velX		= fieldData(mode->advection_stam_velocity.srcVelX);
	float *velY		= fieldData(mode->advection_stam_velocity.srcVelY);
	
	float *velDestX	= fieldData(mode->advection_stam_velocity.dstVelX);
	float *velDestY	= fieldData(mode->advection_stam_velocity.dstVelY);
	
	int sX = fieldStrideX(mode->advection_stam_velocity.srcVelX);
	int sY = fieldStrideY(mode->advection_stam_velocity.srcVelY);
	
	float timestep = mode->advection_stam_velocity.timestep;
	
	//Extract the data from the object
	float bZZ_x, bOZ_x, bZO_x, bOO_x, bZZ_y, bOZ_y, bZO_y, bOO_y;

	//Caching...
	int lastOffX, lastOffY;
	
	//Loop over the data and do the desired computation
	for (x=0; x<w; x++)
	{
		float fDataX = fluidFloatPointer(velX, x*sX + y*sY)[0];
		float fDataY = fluidFloatPointer(velY, x*sX + y*sY)[0];
		
		//Find the cell back in time	(keep a -10,10 radius)
		float backX = -timestep * fDataX + (float)x;
		float backY = -timestep * fDataY + (float)y;
		
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
		
		bZZ_x = fluidFloatPointer(velX, offBackX + offBackY)[0];
		bOZ_x = fluidFloatPointer(velX, offX2 + offBackY)[0];
		bZO_x = fluidFloatPointer(velX, offBackX + offY2)[0];
		bOO_x = fluidFloatPointer(velX, offX2 + offY2)[0];
		
		bZZ_y = fluidFloatPointer(velY, offBackX + offBackY)[0];
		bOZ_y = fluidFloatPointer(velY, offX2 + offBackY)[0];
		bZO_y = fluidFloatPointer(velY, offBackX + offY2)[0];
		bOO_y = fluidFloatPointer(velY, offX2 + offY2)[0];
		
		lastOffX = nBackX;
		lastOffY = nBackY;
		
		fVelDestX[0] = fluidLinearInterpolation(scaleX, scaleY,
										  bZZ_x, bOZ_x, bZO_x, bOO_x);
		fVelDestY[0] = fluidLinearInterpolation(scaleX, scaleY,
										  bZZ_y, bOZ_y, bZO_y, bOO_y);
	}
}
