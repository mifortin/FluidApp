/*
 *  fluid_advection_repos.c
 *  FluidApp
 */

#include "fluid_pvt.h"
#include "fluid_macros_2.h"

//Basic corrector part of predictor-corrector.
void fluid_advection_mccormack_repos(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct mccormack_vel_repos *data = &mode->mccormack_vel_repos;
	
	int x;										//Simple looping var
	int w = fieldWidth(data->srcVelX);			//Width of the data
	int h = fieldHeight(data->srcVelX);
	int sX = fieldStrideX(data->srcVelX);
	int sY = fieldStrideY(data->srcVelY);
	
	float *srcVelX		= fieldData(data->srcVelX);
	float *srcVelY		= fieldData(data->srcVelY);
	
	float *srcErrVelX		= fieldData(data->srcErrVelX);
	float *srcErrVelY		= fieldData(data->srcErrVelY);
	
	float *srcAdvX		= fieldData(data->srcAdvX);
	float *srcAdvY		= fieldData(data->srcAdvY);
	
	float *dstReposX		= fieldData(data->dstReposX);
	float *dstReposY		= fieldData(data->dstReposY);
	
	float *dstVelX		= fieldData(data->dstVelX);
	float *dstVelY		= fieldData(data->dstVelY);
	
	for (x=0; x<w; x++)
	{
		//Compute the addresses of the destinations...
		float *fDstVelX = fluidFloatPointer(dstVelX, x*sX + y*sY);
		float *fDstVelY = fluidFloatPointer(dstVelY, x*sX + y*sY);
		
		float *fDstReposX = fluidFloatPointer(dstReposX, x*sX + y*sY);
		float *fDstReposY = fluidFloatPointer(dstReposY, x*sX + y*sY);
		
		//Get the source values (note that we don't need to scan in memory,
		//so this is much simpler...)
		float fSrcVelX = fluidFloatPointer(srcVelX, x*sX + y*sY)[0];
		float fSrcVelY = fluidFloatPointer(srcVelY, x*sX + y*sY)[0];
		
		float fSrcErrVelX = fluidFloatPointer(srcErrVelX, x*sX + y*sY)[0];
		float fSrcErrVelY = fluidFloatPointer(srcErrVelY, x*sX + y*sY)[0];
		
		float fSrcAdvX = fluidFloatPointer(srcAdvX, x*sX + y*sY)[0];
		float fSrcAdvY = fluidFloatPointer(srcAdvY, x*sX + y*sY)[0];
		
		
		//Do a bit of work... find error
		float errX = fSrcErrVelX - fSrcVelX;
		float errY = fSrcErrVelY - fSrcVelY;
		
		//Apply...
		fDstVelX[0] = fSrcAdvX + errX/2;
		fDstVelY[0] = fSrcAdvY + errY/2;
		
		//Most of the error is from velocity advection (so we assume)
		//	This is a cheap way of computing error!		
		float backX = fluidClamp(-TIMESTEP*srcVelX[0] + errX*TIMESTEP,-9,9) + (float)x;
		float backY = fluidClamp(-TIMESTEP*srcVelY[0] + errY*TIMESTEP,-9,9) + (float)y;
		fDstReposX[0] = fluidClamp(backX,0,w-2);
		fDstReposY[0] = fluidClamp(backY,0,h-2);
	}
}
