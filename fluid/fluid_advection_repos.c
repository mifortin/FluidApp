/*
 *  fluid_advection_repos.c
 *  FluidApp
 */

#include "x_simd.h"

#include "fluid_macros_2.h"
#include "fluid_cpu.h"

//Basic corrector part of predictor-corrector.
void fluid_advection_mccormack_repos(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct mccormack_vel_repos *data = &mode->mccormack_vel_repos;
	
	int x;										//Simple looping var
	int w = fieldWidth(data->srcVelX);			//Width of the data
	int h = fieldHeight(data->srcVelX);
	int sY = fieldStrideY(data->srcVelY);
	float timestep = data->timestep;
	
	int sX = fieldStrideX(data->srcVelX);
	
	float *srcVelX		= fieldData(data->srcVelX);
	float *srcVelY		= fieldData(data->srcVelY);
	
	float *dstReposX		= fieldData(data->dstReposX);
	float *dstReposY		= fieldData(data->dstReposY);
	
	
	int curxy = y*sY;
	for (x=0; x<w; x++)
	{
		//Basic...
		float fVelX = timestep * fluidFloatPointer(srcVelX, curxy)[0];
		float fVelY = timestep * fluidFloatPointer(srcVelY, curxy)[0];
	
		//Advect forward in time...
		float fSrcVelX = fluidClamp(fVelX, -ADVECT_DIST, ADVECT_DIST);
		float fSrcVelY = fluidClamp(fVelY, -ADVECT_DIST, ADVECT_DIST);
		
		//Go forward
		float fwdX = fluidClamp(fSrcVelX + (float)x, 0, w-1.01f);
		float fwdY = fluidClamp(fSrcVelY + (float)y, 0, h-1.01f);
		
		//Get as integer....
		int nBackX = (int)fwdX;
		int nBackY = (int)fwdY;
		
		//Compute scaling factor
		float scaleX = fwdX - (float)nBackX;
		float scaleY = fwdY - (float)nBackY;
				
		//Compute the addresses of the destinations...
		int offBackX = nBackX * sX;
		int offBackY = nBackY * sY;
		int offBackX2 = offBackX + sX;
		int offBackY2 = offBackY + sY;
		
		float bZZ_x = fluidFloatPointer(srcVelX, offBackX + offBackY)[0];
		float bOZ_x = fluidFloatPointer(srcVelX, offBackX2 + offBackY)[0];
		float bZO_x = fluidFloatPointer(srcVelX, offBackX + offBackY2)[0];
		float bOO_x = fluidFloatPointer(srcVelX, offBackX2 + offBackY2)[0];
		
		float bZZ_y = fluidFloatPointer(srcVelY, offBackX + offBackY)[0];
		float bOZ_y = fluidFloatPointer(srcVelY, offBackX2 + offBackY)[0];
		float bZO_y = fluidFloatPointer(srcVelY, offBackX + offBackY2)[0];
		float bOO_y = fluidFloatPointer(srcVelY, offBackX2 + offBackY2)[0];
		
		//Compute the velocity at that point...
		float fwdVelX = fluidLinearInterpolation(scaleX, scaleY, bZZ_x, bOZ_x, bZO_x, bOO_x);
		float fwdVelY = fluidLinearInterpolation(scaleX, scaleY, bZZ_y, bOZ_y, bZO_y, bOO_y);
		
		//This velocity (computed the other way) will give spatial error
		float fwdBackX = fluidClamp(-fwdVelX * timestep, -ADVECT_DIST, ADVECT_DIST) + fwdX;
		float fwdBackY = fluidClamp(-fwdVelY * timestep, -ADVECT_DIST, ADVECT_DIST) + fwdY;
		
		//Use spatial error to compute where the repos should fall
		float errX = fwdBackX - (float)x;
		float errY = fwdBackY - (float)y;
		
		
		float *fDstReposX = fluidFloatPointer(dstReposX, curxy);
		float *fDstReposY = fluidFloatPointer(dstReposY, curxy);
		
		//Most of the error is from velocity advection (so we assume)
		//	This is a cheap way of computing error!
		float backX = fluidClamp(-fVelX + 0.5f*errX,-ADVECT_DIST,ADVECT_DIST) + (float)x;
		fDstReposX[0] = fluidClamp(backX,0,w-1.01f);
		
		float backY = fluidClamp(-fVelY + 0.5f*errY,-ADVECT_DIST,ADVECT_DIST) + (float)y;
		fDstReposY[0] = fluidClamp(backY,0,w-1.01f);
		
		curxy += sX;
	}
}
