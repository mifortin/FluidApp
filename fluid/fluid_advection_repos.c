/*
 *  fluid_advection_repos.c
 *  FluidApp
 */
#ifdef __SSE3__
#include <xmmintrin.h>
#include <emmintrin.h>
#include <pmmintrin.h>
//#undef __SSE3__
#endif

#ifdef CELL
#include "altivec.h"
#endif

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
	
#ifdef __APPLE_ALTIVEC__
	int curxy = y*sY;
	
	
	vector float *srcVelX		= (vector float *)fluidFloatPointer(fieldData(data->srcVelX), curxy);
	vector float *srcVelY		= (vector float *)fluidFloatPointer(fieldData(data->srcVelY), curxy);
	
	vector float *srcErrVelX	= (vector float *)fluidFloatPointer(fieldData(data->srcErrVelX), curxy);
	vector float *srcErrVelY	= (vector float *)fluidFloatPointer(fieldData(data->srcErrVelY), curxy);
	
	vector float *srcAdvX		= (vector float *)fluidFloatPointer(fieldData(data->srcAdvX), curxy);
	vector float *srcAdvY		= (vector float *)fluidFloatPointer(fieldData(data->srcAdvY), curxy);
	
	vector float *dstReposX		= (vector float *)fluidFloatPointer(fieldData(data->dstReposX), curxy);
	vector float *dstReposY		= (vector float *)fluidFloatPointer(fieldData(data->dstReposY), curxy);
	
	vector float *dstVelX		= (vector float *)fluidFloatPointer(fieldData(data->dstVelX), curxy);
	vector float *dstVelY		= (vector float *)fluidFloatPointer(fieldData(data->dstVelY), curxy);
	
	vector float vHalf = {0.5f, 0.5f, 0.5f, 0.5f};
	vector float negTimestep = {-timestep, -timestep, -timestep, -timestep};
	vector float vTimestep = {timestep, timestep, timestep, timestep};
	vector float vZero = {0,0,0,0};
	
	vector float vNine = {ADVECT_DIST,ADVECT_DIST,ADVECT_DIST,ADVECT_DIST};
	vector float vNegNine = {-ADVECT_DIST,-ADVECT_DIST,-ADVECT_DIST,-ADVECT_DIST};
	vector float wm2 = {w-2,w-2,w-2,w-2};
	vector float hm2 = {h-2,h-2,h-2,h-2};
	
	vector float vOne = {4,4,4,4};
	vector float v1234 = {0,1,2,3};
	
	w/=4;
	
	for (x=0; x<w; x++)
	{
		vector float errX = vec_sub(srcErrVelX[x], srcVelX[x]);
		dstVelX[x] = vec_madd(vHalf, errX, srcAdvX[x]);
	}
	
	for (x=0; x<w; x++)
	{		
		vector float tmp = vec_madd(
								negTimestep,
								srcVelX[x],
								vec_madd(
									vTimestep,
									vec_madd(vHalf,vec_sub(srcErrVelX[x],srcVelX[x]),vZero),
									 vZero));
		
		tmp = vec_min(vNine, tmp);
		tmp = vec_max(vNegNine, tmp);
		
		tmp = vec_add(tmp, v1234);
		v1234 = vec_add(v1234, vOne);
		
		tmp = vec_min(tmp, wm2);
		dstReposX[x] = vec_max(tmp, vZero);
	}
	
	for (x=0; x<w; x++)
	{
		vector float errY = vec_sub(srcErrVelY[x], srcVelY[x]);
		dstVelY[x] = vec_madd(vHalf, errY, srcAdvY[x]);
	}
	
	vector float vY = {y,y,y,y};
	for (x=0; x<w; x++)
	{
		vector float tmp = vec_madd(
						negTimestep,
						srcVelY[x],
						vec_madd(
							 vTimestep,
							 vec_madd(vHalf,vec_sub(srcErrVelY[x],srcVelY[x]),vZero),
							 vZero));
		
		tmp = vec_min(vNine, tmp);
		tmp = vec_max(vNegNine, tmp);
		
		tmp = vec_add(tmp, vY);
		
		tmp = vec_min(tmp, hm2);
		dstReposY[x] = vec_max(tmp, vZero);
	}

#elif defined __SSE3__
	
	int curxy = y*sY;
	
	__m128 *srcVelX		= (__m128 *)fluidFloatPointer(fieldData(data->srcVelX), curxy);
	__m128 *srcVelY		= (__m128 *)fluidFloatPointer(fieldData(data->srcVelY), curxy);
	
	__m128 *srcErrVelX	= (__m128 *)fluidFloatPointer(fieldData(data->srcErrVelX), curxy);
	__m128 *srcErrVelY	= (__m128 *)fluidFloatPointer(fieldData(data->srcErrVelY), curxy);
	
	__m128 *srcAdvX		= (__m128 *)fluidFloatPointer(fieldData(data->srcAdvX), curxy);
	__m128 *srcAdvY		= (__m128 *)fluidFloatPointer(fieldData(data->srcAdvY), curxy);
	
	__m128 *dstReposX		= (__m128 *)fluidFloatPointer(fieldData(data->dstReposX), curxy);
	__m128 *dstReposY		= (__m128 *)fluidFloatPointer(fieldData(data->dstReposY), curxy);
	
	__m128 *dstVelX		= (__m128 *)fluidFloatPointer(fieldData(data->dstVelX), curxy);
	__m128 *dstVelY		= (__m128 *)fluidFloatPointer(fieldData(data->dstVelY), curxy);
	
	__m128 vHalf = {0.5f, 0.5f, 0.5f, 0.5f};
	__m128 negTimestep = {-timestep, -timestep, -timestep, -timestep};
	__m128 vTimestep = {timestep, timestep, timestep, timestep};
	__m128 vZero = {0,0,0,0};
	
	__m128 vNine = {ADVECT_DIST,ADVECT_DIST,ADVECT_DIST,ADVECT_DIST};
	__m128 vNegNine = {-ADVECT_DIST,-ADVECT_DIST,-ADVECT_DIST,-ADVECT_DIST};
	__m128 wm2 = {w-2,w-2,w-2,w-2};
	__m128 hm2 = {h-2,h-2,h-2,h-2};
	
	__m128 vOne = {4,4,4,4};
	__m128 v1234 = {0,1,2,3};
	
	w/=4;
	
	for (x=0; x<w; x++)
	{
		__m128 errX = _mm_sub_ps(srcErrVelX[x], srcVelX[x]);
		dstVelX[x] = _mm_add_ps(_mm_mul_ps(vHalf, errX), srcAdvX[x]);
	}
	
	for (x=0; x<w; x++)
	{		
		__m128 tmp = _mm_add_ps(_mm_mul_ps(
							negTimestep,
							srcVelX[x]),
							_mm_mul_ps(
									 vTimestep,
									 _mm_mul_ps(
											  vHalf,
											  _mm_sub_ps(srcErrVelX[x],srcVelX[x]))
									));
		
		tmp = _mm_min_ps(tmp, vNine);
		tmp = _mm_max_ps(tmp, vNegNine);
		
		tmp = _mm_add_ps(tmp, v1234);
		v1234 = _mm_add_ps(v1234, vOne);
		
		tmp = _mm_min_ps(tmp, wm2);
		dstReposX[x] = _mm_max_ps(tmp, vZero);
	}
	
	for (x=0; x<w; x++)
	{
		__m128 errY = _mm_sub_ps(srcErrVelY[x], srcVelY[x]);
		dstVelY[x] = _mm_add_ps(_mm_mul_ps(vHalf, errY), srcAdvY[x]);
	}
	
	__m128 vY = {y,y,y,y};
	for (x=0; x<w; x++)
	{
		__m128 tmp = _mm_add_ps(_mm_mul_ps(
								negTimestep,
								srcVelY[x]),
								_mm_mul_ps(
									 vTimestep,
									 _mm_mul_ps(
										vHalf,
									_mm_sub_ps(srcErrVelY[x],srcVelY[x]))));
		
		tmp = _mm_min_ps(tmp, vNine);
		tmp = _mm_max_ps(tmp, vNegNine);
		
		tmp = _mm_add_ps(tmp, vY);
		
		tmp = _mm_min_ps(tmp, hm2);
		dstReposY[x] = _mm_max_ps(tmp, vZero);
	}
	
#else
	int sX = fieldStrideX(data->srcVelX);
	
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
	
	
	int curxy = y*sY;
	for (x=0; x<w; x++)
	{
		//Compute the addresses of the destinations...
		float *fDstVelX = fluidFloatPointer(dstVelX, curxy);
		
		//Get the source values (note that we don't need to scan in memory,
		//so this is much simpler...)
		float fSrcVelX = fluidFloatPointer(srcVelX, curxy)[0];
		
		float fSrcErrVelX = fluidFloatPointer(srcErrVelX, curxy)[0];
		
		float fSrcAdvX = fluidFloatPointer(srcAdvX, curxy)[0];
		
		
		//Do a bit of work... find error
		float errX = fSrcErrVelX - fSrcVelX;
		
		//Apply... (this puts fluid at wrong timestep if done before!)
		fDstVelX[0] = fSrcAdvX + errX/2;
		
		curxy += sX;
	}
	
	curxy = y*sY;
	for (x=0; x<w; x++)
	{
		//Compute the addresses of the destinations...
		
		float *fDstReposX = fluidFloatPointer(dstReposX, curxy);
		
		//Get the source values (note that we don't need to scan in memory,
		//so this is much simpler...)
		float fSrcVelX = fluidFloatPointer(srcVelX, curxy)[0];
		
		float fSrcErrVelX = fluidFloatPointer(srcErrVelX, curxy)[0];
		
		
		//Do a bit of work... find error
		float errX = fSrcErrVelX - fSrcVelX;
		
		//Most of the error is from velocity advection (so we assume)
		//	This is a cheap way of computing error!
		float backX = fluidClamp(-timestep*fSrcVelX + 0.5f*errX*timestep,-ADVECT_DIST,ADVECT_DIST) + (float)x;
		fDstReposX[0] = fluidClamp(backX,0,w-2);
		
		curxy += sX;
	}
	
	curxy = y*sY;
	for (x=0; x<w; x++)
	{
		//Compute the addresses of the destinations...
		float *fDstVelY = fluidFloatPointer(dstVelY, curxy);
		
		//Get the source values (note that we don't need to scan in memory,
		//so this is much simpler...)
		float fSrcVelY = fluidFloatPointer(srcVelY, curxy)[0];
		
		float fSrcErrVelY = fluidFloatPointer(srcErrVelY, curxy)[0];
		
		float fSrcAdvY = fluidFloatPointer(srcAdvY, curxy)[0];
		
		
		//Do a bit of work... find error
		float errY = fSrcErrVelY - fSrcVelY;
		
		//Apply... (this puts fluid at wrong timestep if done before!)
		fDstVelY[0] = fSrcAdvY + errY/2;
		
		curxy += sX;
	}
	
	curxy = y*sY;
	for (x=0; x<w; x++)
	{
		
		float *fDstReposY = fluidFloatPointer(dstReposY, curxy);
		
		//Get the source values (note that we don't need to scan in memory,
		//so this is much simpler...)
		float fSrcVelY = fluidFloatPointer(srcVelY, curxy)[0];
		
		float fSrcErrVelY = fluidFloatPointer(srcErrVelY, curxy)[0];
		
		
		//Do a bit of work... find error
		float errY = fSrcErrVelY - fSrcVelY;
		
		//Most of the error is from velocity advection (so we assume)
		//	This is a cheap way of computing error!
		float backY = fluidClamp(-timestep*fSrcVelY + 0.5f*errY*timestep,-ADVECT_DIST,ADVECT_DIST) + (float)y;
		fDstReposY[0] = fluidClamp(backY,0,h-2);
		
		curxy += sX;
	}
#endif
}
