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
	
	float *repX		= fieldData(in_f->r_reposX);
	float *repY		= fieldData(in_f->r_reposY);
	
	int sX = fieldStrideX(in_f->r_velocityX);
	int sY = fieldStrideY(in_f->r_velocityY);
	
	for (x=0; x<w; x++)
	{
		//We first repeat all the steps but will apply a corrector to
		//the mess...
		float *fDataX = fluidFloatPointer(velX, x*sX + y*sY);
		float *fDataY = fluidFloatPointer(velY, x*sX + y*sY);
		
		//Find the cell back in time	(keep a -10,10 radius)
		float backX = -fluidClamp(TIMESTEP * fDataX[0],-9.0f,9.0f) + (float)x;
		float backY = -fluidClamp(TIMESTEP * fDataY[0],-9.0f,9.0f) + (float)y;
		
		//Extract the old value....
		float f_vel_x = fluidFloatPointer(velDestX, x*sX + y*sY)[0];
		float f_vel_y = fluidFloatPointer(velDestY, x*sX + y*sY)[0];
		
		//Now we recompute the backX and backY - but for the correction
		//data (notice that timestep is reversed - so we should return
		//		back to the start position)
		float new_backX = fluidClamp(TIMESTEP*f_vel_x,-9.0f,9.0f) + backX;
		float new_backY = fluidClamp(TIMESTEP*f_vel_y,-9.0f,9.0f) + backY;
		
		int new_nBackX = (int)new_backX;
		int new_nBackY = (int)new_backY;
		
		float new_scaleX = new_backX - (float)new_nBackX;
		float new_scaleY = new_backY - (float)new_nBackY;
		
		new_nBackX = fluidClamp(new_nBackX, 0,w-2);
		new_nBackY = fluidClamp(new_nBackY, 0,h-2);
		
		//Fortuneately, we cut quite a few calculations by just advecting
		//a repos matrix.
		//Also cut down on quite a few conditionals...
		
		//A few notes:	stam's values are stored in (backX) and (backY)
		//				old value is found in the (x) and (y)
		float *fReposDestX = fluidFloatPointer(repX, sX*x + sY*y);
		float *fReposDestY = fluidFloatPointer(repY, sX*x + sY*y);
		
		//Offsets from the output (predictor-corrector)
		int outBackX = new_nBackX * sX;
		int outBackY = new_nBackY * sY;
		int outX2 = (new_nBackX+1)*sX;
		int outY2 = (new_nBackY+1)*sY;
		
		float *outZZ_x = fluidFloatPointer(velDestX, outBackX + outBackY);
		float *outOZ_x = fluidFloatPointer(velDestX, outX2 + outBackY);
		float *outZO_x = fluidFloatPointer(velDestX, outBackX + outY2);
		float *outOO_x = fluidFloatPointer(velDestX, outX2 + outY2);
		
		float *outZZ_y = fluidFloatPointer(velDestY, outBackX + outBackY);
		float *outOZ_y = fluidFloatPointer(velDestY, outX2 + outBackY);
		float *outZO_y = fluidFloatPointer(velDestY, outBackX + outY2);
		float *outOO_y = fluidFloatPointer(velDestY, outX2 + outY2);
		
		float interpX = fluidLinearInterpolation(new_scaleX, new_scaleY,
											outZZ_x[0], outOZ_x[0], outZO_x[0], outOO_x[0]);
		float interpY = fluidLinearInterpolation(new_scaleX, new_scaleY,
											outZZ_y[0], outOZ_y[0], outZO_y[0], outOO_y[0]);
		
		float xError = interpX-fDataX[0];
		fReposDestX[0] = f_vel_x + 0.5f * xError;
		
		float yError = interpY-fDataY[0];
		fReposDestY[0] = f_vel_y + 0.5f * yError;
	}
}
