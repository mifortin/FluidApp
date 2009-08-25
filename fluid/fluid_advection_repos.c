/*
 *  fluid_advection_repos.c
 *  FluidApp
 */

#include "fluid_pvt.h"
#include "fluid_macros_2.h"

void fluid_advection_mccormack_repos(fluid *in_f, int y)
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
	
	for (x=0; x<w; x++)
	{
		//We first repeat all the steps but will apply a corrector to
		//the mess...
		float *fDataX = fluidFloatPointer(velX, x*sX + y*sY);
		float *fDataY = fluidFloatPointer(velY, x*sX + y*sY);
		
		//Find the cell back in time	(keep a -10,10 radius)
		float backX = -fluidClamp(TIMESTEP * fDataX[0],-9,9) + (float)x;
		float backY = -fluidClamp(TIMESTEP * fDataY[0],-9,9) + (float)y;
		
		int nBackX = (int)backX;
		int nBackY = (int)backY;
		
		//Clamp as it's easier to parallelize given the scheduler
		nBackX = fluidClamp(nBackX, 0,w-2);
		nBackY = fluidClamp(nBackY, 0,h-2);
		
		//Extract the old value....
		float f_vel_x = fluidFloatPointer(velDestX, x*sX + y*sY)[0];
		float f_vel_y = fluidFloatPointer(velDestY, x*sX + y*sY)[0];
		
		//Now we recompute the backX and backY - but for the correction
		//data (notice that timestep is reversed - so we should return
		//		back to the start position)
		float new_backX = TIMESTEP*f_vel_x + backX;
		float new_backY = TIMESTEP*f_vel_y + backY;
		
		int new_nBackX = (int)new_backX;
		int new_nBackY = (int)new_backY;
		
		float new_scaleX = new_backX - (float)new_nBackX;
		float new_scaleY = new_backY - (float)new_nBackY;
		
		FSWrap(new_nBackX, new_scaleX, in_vel.width);
		FSWrap(new_nBackY, new_scaleY, in_vel.height);
		
		FSWrapPlus1(new_nBackX, new_x2, new_scaleX, in_vel.width);
		FSWrapPlus1(new_nBackY, new_y2, new_scaleY, in_vel.height);
		
		//Fortuneately, we cut quite a few calculations by just advecting
		//a repos matrix.
		//Also cut down on quite a few conditionals...
		
		//A few notes:	stam's values are stored in (backX) and (backY)
		//				old value is found in the (x) and (y)
		float *fReposDest = FSFloatPtrOffset(out_repos.data.f,
											 out_repos.strideX*x + out_repos.strideY*y);
		
		//Offsets from the output (predictor-corrector)
		int outBackX = new_nBackX * tmp_repos.strideX;
		int outBackY = new_nBackY * tmp_repos.strideY;
		int outX2 = new_x2 * tmp_repos.strideX;
		int outY2 = new_y2 * tmp_repos.strideY;
		
		float *outZZ = FSFloatPtrOffset(tmp_repos.data.f,
										outBackX + outBackY);
		float *outOZ = FSFloatPtrOffset(tmp_repos.data.f,
										outX2 + outBackY);
		float *outZO = FSFloatPtrOffset(tmp_repos.data.f,
										outBackX + outY2);
		float *outOO = FSFloatPtrOffset(tmp_repos.data.f,
										outX2 + outY2);
		
		float interpX = LinearInterpolation(new_scaleX, new_scaleY,
											outZZ[0], outOZ[0], outZO[0], outOO[0]);
		float interpY = LinearInterpolation(new_scaleX, new_scaleY,
											outZZ[1], outOZ[1], outZO[1], outOO[1]);
		
		float xError = (float)x-interpX;
		if (fabsf(xError) > fabsf(((float)x+in_vel.width) - interpX))
			xError = ((float)x+in_vel.width) - interpX;
		if (fabsf(xError) > fabsf(((float)x-in_vel.width) - interpX))
			xError = ((float)x-in_vel.width) - interpX;
		
		fReposDest[0] = backX + 0.5f * xError;
		//Clamp(fReposDest[0], backX-1, backX+1);
		
		
		float yError = (float)y-interpY;
		if (fabsf(yError) > fabsf((float)(y+in_vel.height) - interpY))
			yError = (float)(y+in_vel.height) - interpY;
		if (fabsf(yError) > fabsf((float)(y-in_vel.height) - interpY))
			yError = (float)(y-in_vel.height) - interpY;
		
		fReposDest[1] = backY + 0.5f * yError;
	}
}
