/*
 *  fluid_advection_repos.c
 *  FluidApp
 */

#include "x_simd.h"

#include "fluid_macros_2.h"
#include "fluid_cpu.h"
#include <stdio.h>

//Basic corrector part of predictor-corrector.
void fluid_advection_mccormack_repos(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct mccormack_vel_repos *data = &mode->mccormack_vel_repos;
	
	int x;										//Simple looping var
	int w = fieldWidth(data->srcVelX);			//Width of the data
	int h = fieldHeight(data->srcVelX);
	int sY = fieldStrideY(data->srcVelY);
	x128f timestep = {data->timestep,data->timestep,data->timestep,data->timestep};
	
	int sX = fieldStrideX(data->srcVelX);
	
	x128i vsX = {sX, sX, sX, sX};
	x128i vsY = {sY, sY, sY, sY};
	
	float *srcVelX		= fieldData(data->srcVelX);
	float *srcVelY		= fieldData(data->srcVelY);
	
	float *dstReposX		= fieldData(data->dstReposX);
	float *dstReposY		= fieldData(data->dstReposY);
	
	
	int curxy = y*sY;
	
	int w2 = w/4;
	//printf("%i %i\n", w2, w);
	
	x128f vMin = {-ADVECT_DIST, -ADVECT_DIST, -ADVECT_DIST, -ADVECT_DIST};
	x128f vMax = {ADVECT_DIST, ADVECT_DIST, ADVECT_DIST, ADVECT_DIST};
	
	x128f *vSrcVelX = (x128f*)fluidFloatPointer(srcVelX, curxy);
	x128f *vSrcVelY = (x128f*)fluidFloatPointer(srcVelY, curxy);
	
	x128f *vDstReposX = (x128f*)fluidFloatPointer(dstReposX, curxy);
	x128f *vDstReposY = (x128f*)fluidFloatPointer(dstReposY, curxy);
	
	x128f vX = {0.0f, 1.0f, 2.0f, 3.0f};
	x128f vY = {y, y, y, y};
	
	x128f vNHalf = {-0.5f,-0.5f,-0.5f,-0.5f};
	x128f v4 = {4.0f, 4.0f, 4.0f, 4.0f};
	x128f vN1 = {-1.0f, -1.0f, -1.0f, -1.0f};
	
	x128f vW = {w-1.01f,w-1.01f,w-1.01f,w-1.01f};
	x128f vH = {h-1.01f,h-1.01f,h-1.01f,h-1.01f};
	
	u128i offBackX, offBackY, offBackX2, offBackY2;
	u128f scaleX, scaleY, fwdVelX, fwdVelY;
	float bZZ, bOZ, bZO, bOO;
	for (x=0; x<w2; x++)
	{
		//Basic...
		x128f fVelX = x_mul(timestep, vSrcVelX[x]);
		x128f fVelY = x_mul(timestep, vSrcVelY[x]);
	
		//Advect forward in time...
		x128f fSrcVelX = x_clamp(fVelX, vMin, vMax);
		x128f fSrcVelY = x_clamp(fVelY, vMin, vMax);
		
		//Go forward
		x128f fwdX = x_clamp(x_add(fSrcVelX, vX), x_simd_zero, vW);
		x128f fwdY = x_clamp(x_add(fSrcVelY, vY), x_simd_zero, vH);
		
		//Back as integer
		x128i nBackX = x_ftoi(fwdX);
		x128i nBackY = x_ftoi(fwdY);
		
		//Compute scaling factor
		scaleX.v = x_sub(fwdX, x_itof(nBackX));
		scaleY.v = x_sub(fwdY, x_itof(nBackY));
				
		//Compute the addresses of the destinations...
		offBackX.v = x_imul(nBackX, vsX);
		offBackY.v = x_imul(nBackY, vsY);
		offBackX2.v = x_iadd(offBackX.v, vsX);
		offBackY2.v = x_iadd(offBackY.v, vsY);
		
		int i;
		for (i=0; i<4; i++)
		{
			bZZ = fluidFloatPointer(srcVelX, offBackX.i[i] + offBackY.i[i])[0];
			bOZ = fluidFloatPointer(srcVelX, offBackX2.i[i] + offBackY.i[i])[0];
			bZO = fluidFloatPointer(srcVelX, offBackX.i[i] + offBackY2.i[i])[0];
			bOO = fluidFloatPointer(srcVelX, offBackX2.i[i] + offBackY2.i[i])[0];
			
			fwdVelX.f[i] = fluidLinearInterpolation(scaleX.f[i], scaleY.f[i],bZZ, bOZ, bZO, bOO);
			
			
			bZZ = fluidFloatPointer(srcVelY, offBackX.i[i] + offBackY.i[i])[0];
			bOZ = fluidFloatPointer(srcVelY, offBackX2.i[i] + offBackY.i[i])[0];
			bZO = fluidFloatPointer(srcVelY, offBackX.i[i] + offBackY2.i[i])[0];
			bOO = fluidFloatPointer(srcVelY, offBackX2.i[i] + offBackY2.i[i])[0];
			fwdVelY.f[i] = fluidLinearInterpolation(scaleX.f[i], scaleY.f[i],bZZ, bOZ, bZO, bOO);
		}
		
		//Compute the velocity at that point...
		
		//This velocity (computed the other way) will give spatial error
		x128f errX = x_clamp(x_mul(x_mul(vN1, timestep), fwdVelX.v), vMin, vMax);
		x128f errY = x_clamp(x_mul(x_mul(vN1, timestep), fwdVelY.v), vMin, vMax);
	
		
		//Most of the error is from velocity advection (so we assume)
		//	This is a cheap way of computing error!
		x128f backX = x_add(x_clamp(x_madd(vNHalf,errX,x_mul(fVelX,vN1)),vMin,vMax), vX);
		vDstReposX[x] = x_clamp(backX,x_simd_zero, vW);
		
		x128f backY = x_add(x_clamp(x_madd(vNHalf,errY,x_mul(fVelY,vN1)),vMin,vMax), vY);
		vDstReposY[x] = x_clamp(backY,x_simd_zero, vH);
		
		vX = x_add(vX, v4);
	}
}
