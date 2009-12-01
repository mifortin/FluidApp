/*
 *  fluid_vorticity.c
 *  FluidApp
 */

#ifdef __SSE3__
#include <xmmintrin.h>
#include <emmintrin.h>
#include <pmmintrin.h>
//#undef __SSE3__
#endif

#include "fluid_macros_2.h"
#include <math.h>
#include <stdlib.h>
#include "fluid_cpu.h"

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
		float dydx = 0;
		float dxdy = 0;
		fluidFloatPointer(z, sx*x + sy*y)[0] = (dydx - dxdy);
		
		for (x=1; x<w-1; x++)
		{
			dydx = (fluidFloatPointer(velY, (x+1)*sx)[0]
					- fluidFloatPointer(velY, (x-1)*sx)[0])/2;
			dxdy = 0;
			fluidFloatPointer(z, sx*x + sy*y)[0] = (dydx - dxdy);
		}
		
		dydx = 0;
		dxdy = 0;
		fluidFloatPointer(z, sx*x + sy*y)[0] = (dydx - dxdy);
	}
	else if (y == h-1)
	{
		x = 0;
		float dydx = 0;
		float dxdy = 0;
		fluidFloatPointer(z, sx*x + sy*y)[0] = (dydx - dxdy);
		
		for (x=1; x<w-1; x++)
		{
			dydx = (fluidFloatPointer(velY, (x+1)*sx + y*sy)[0]
					- fluidFloatPointer(velY, (x-1)*sx + y*sy)[0])/2;
			dxdy = 0;
			fluidFloatPointer(z, sx*x + sy*y)[0] = (dydx - dxdy);
		}
		
		dydx = 0;
		dxdy = 0;
		fluidFloatPointer(z, sx*x + sy*y)[0] = (dydx - dxdy);
	}
	else
	{
		x = 0;
		float dydx = 0;
		float dxdy = (fluidFloatPointer(velX, sx + (y+1)*sy)[0]
					  - fluidFloatPointer(velX, sx + (y-1)*sy)[0])/2;
		fluidFloatPointer(z, sx*x + sy*y)[0] = (dydx - dxdy);
		
		for (x=1; x<w-1; x++)
		{
			dydx = (fluidFloatPointer(velY, (x+1)*sx + y*sy)[0]
						  - fluidFloatPointer(velY, (x-1)*sx + y*sy)[0])/2;
			dxdy = (fluidFloatPointer(velX, sx + (y+1)*sy)[0]
						  - fluidFloatPointer(velX, sx + (y-1)*sy)[0])/2;
			fluidFloatPointer(z, sx*x + sy*y)[0] = (dydx - dxdy);
		}
		
		dydx = 0;
		dxdy = (fluidFloatPointer(velX, sx + (y+1)*sy)[0]
				- fluidFloatPointer(velX, sx + (y-1)*sy)[0])/2;
		fluidFloatPointer(z, sx*x + sy*y)[0] = (dydx - dxdy);
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
//#undef __SSE3__
#ifdef __SSE3__
		int vW = w/4;
		
		__m128 *vZ = (__m128*)fluidFloatPointer(z, y*sy);
		
		__m128 *vVelX = (__m128*)fluidFloatPointer(velX, y*sy);
		__m128 *vVelY = (__m128*)fluidFloatPointer(velY, y*sy);
		
		__m128 *vZP = (__m128*)fluidFloatPointer(z, (y-1)*sy);
		__m128 *vZN = (__m128*)fluidFloatPointer(z, (y+1)*sy);
		
		__m128i absMask = {0x7FFFFFFF7FFFFFFF,0x7FFFFFFF7FFFFFFF};
		
		__m128 smallValue = {0.001f, 0.001f, 0.001f, 0.001f};
		__m128 vE = {e,e,e,e};
		
		__m128 vNegNine = {-9,-9,-9,-9};
		__m128 vNine = {9,9,9,9};
		
		__m128 vHalf = {0.5f, 0.5f, 0.5f, 0.5f};
		
		for (x=1; x<vW-1; x++)
		{
			__m128 sl = _mm_srli_sf128(vZ[x], 4);
			sl = _mm_add_ps(sl, _mm_slli_sf128(vZ[x+1],12));
			
			__m128 sr = _mm_slli_sf128(vZ[x], 4);
			sr = _mm_add_ps(sr, _mm_srli_sf128(vZ[x-1], 12));
			
			__m128 dzdx = _mm_mul_ps(vHalf,_mm_sub_ps(_mm_and_ps((__m128)absMask,sl),_mm_and_ps((__m128)absMask,sr)));
			__m128 dzdy = _mm_mul_ps(vHalf,_mm_sub_ps(_mm_and_ps((__m128)absMask,vZP[x]), _mm_and_ps((__m128)absMask,vZN[x])));
			
			__m128 mag = _mm_add_ps(_mm_mul_ps(dzdx,dzdx), _mm_mul_ps(dzdy,dzdy));
			
			__m128 magMask = _mm_cmpgt_ps(mag,smallValue);
			//int *mm = (int*)&magMask;
			//if (mm[0] || mm[1] || mm[2] || mm[3])
			{
				mag = _mm_mul_ps(vE,_mm_rsqrt_ps(mag));
				
				dzdx = _mm_mul_ps(dzdx,mag);
				dzdy = _mm_mul_ps(dzdy,mag);
				
				dzdx = _mm_and_ps(dzdx, magMask);
				dzdy = _mm_and_ps(dzdy, magMask);
				
				vVelX[x] = _mm_add_ps(vVelX[x],_mm_mul_ps(dzdy,vZ[x]));
				vVelX[x] = _mm_max_ps(vVelX[x], vNegNine);
				vVelX[x] = _mm_min_ps(vVelX[x], vNine);
				
				vVelY[x] = _mm_sub_ps(vVelY[x],_mm_mul_ps(dzdx,vZ[x]));
				vVelY[x] = _mm_max_ps(vVelY[x], vNegNine);
				vVelY[x] = _mm_min_ps(vVelY[x], vNine);
			}
		}
#else
		
		float dzdx, dzdy, mag;
		for (x=1; x<w-1; x++)
		{
			dzdx = (fabsf(fluidFloatPointer(z, (x+1)*sx + y*sy)[0])
					- fabsf(fluidFloatPointer(z, (x-1)*sx + y*sy)[0]))/2;
			dzdy = (fabsf(fluidFloatPointer(z, sx + (y+1)*sy)[0])
					- fabsf(fluidFloatPointer(z, sx + (y-1)*sy)[0]))/2;
			float mg = dzdx*dzdx + dzdy*dzdy;
			if (mg > 0.001f)
			{
				mag = 1.0f/sqrtf(mg);
				dzdx= e*dzdx * mag;
				dzdy= e*dzdy * mag;
				
				fluidFloatPointer(velX, x*sx + sy*y)[0]
							+= dzdy *fluidFloatPointer(z, x*sx+ y*sy)[0];
				
				*fluidFloatPointer(velX,x*sx + y*sy)
						= fluidClamp(*fluidFloatPointer(velX,x*sx + y*sy),-9,9);
			}
		}
		
		for (x=1; x<w-1; x++)
		{
			dzdx = (fabsf(fluidFloatPointer(z, (x+1)*sx + y*sy)[0])
					- fabsf(fluidFloatPointer(z, (x-1)*sx + y*sy)[0]))/2;
			dzdy = (fabsf(fluidFloatPointer(z, sx + (y+1)*sy)[0])
					- fabsf(fluidFloatPointer(z, sx + (y-1)*sy)[0]))/2;
			float mg = dzdx*dzdx + dzdy*dzdy;
			if (mg > 0.001f)
			{
				mag = 1.0f/sqrtf(mg);
				dzdx= e*dzdx * mag;
				dzdy= e*dzdy * mag;
				
				fluidFloatPointer(velY, x*sx + sy*y)[0]
						-= dzdx *fluidFloatPointer(z, x*sx+ y*sy)[0];
				
				*fluidFloatPointer(velY,x*sx + y*sy)
						= fluidClamp(*fluidFloatPointer(velY,x*sx + y*sy),-9,9);
			}
		}
#endif
	}
}
