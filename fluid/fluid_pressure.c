/*
 *  fluid_pressure.c
 *  FluidApp
 */

#ifdef __SSE3__
#include <xmmintrin.h>
#include <emmintrin.h>
#include <pmmintrin.h>
//#undef __SSE3__
#endif

#include "fluid_macros_2.h"
#include "fluid_cpu.h"

#include <math.h>
#include <stdio.h>

/** Most basic function used to handle pressure */
void fluid_genPressure(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct pressure *p = &mode->pressure;
	
	int w = fieldWidth(p->velX);
	int h = fieldHeight(p->velX);
	
	int sx = fieldStrideX(p->velX);
	int sy = fieldStrideY(p->velY);
	
	float *velX = fieldData(p->velX);
	float *velY = fieldData(p->velY);
	
	float *pressure = fieldData(p->pressure);
	
	if (y == 0)
	{
		int x;
		for (x=0; x<w; x++)
		{
			fluidFloatPointer(pressure,x*sx)[0] = fluidFloatPointer(pressure,x*sx + sy)[0];
		}
	}
	else if (y == h-1)
	{
		int x;
		for (x=0; x<w; x++)
		{
			fluidFloatPointer(pressure,x*sx + y*sy)[0] =
					fluidFloatPointer(pressure,x*sx + (y-1)*sy)[0];
		}
	}
	else
	{
#ifdef __APPLE_ALTIVEC__
		float *vPressureRow = fluidFloatPointer(pressure, y*sy);
		
		vector float *vPressure = (vector float*)vPressureRow;
		vector float *vVelX = (vector float*)fluidFloatPointer(velX, y*sy);
		
		vector float *vPressureN = (vector float*)fluidFloatPointer(pressure, (y+1)*sy);
		vector float *vVelYN = (vector float*)fluidFloatPointer(velY, (y+1)*sy);
		
		vector float *vPressureP = (vector float*)fluidFloatPointer(pressure, (y-1)*sy);
		vector float *vVelYP = (vector float*)fluidFloatPointer(velY, (y-1)*sy);
		
		vector float div4 = {1.0f/4.0f, 1.0f/4.0f, 1.0f/4.0f, 1.0f/4.0f};
		vector float vZero = {0,0,0,0};
		
		vec_dstst(vPressure, 0x01000001, 0);
		vec_dst(vVelX, 0x01000001, 1);
		vec_dst(vVelYN, 0x01000001, 2);
		vec_dst(vVelYP, 0x01000001, 3);
		
		int x;
		{
			vector float tmp;
			
			//Compute shifts
			vector float sl_p = vec_sld(vPressure[0], vPressure[1],4);
			vector float sr_p = vec_sld(vZero, vPressure[0], 12);
			
			vector float sl_vx = vec_sld(vVelX[0], vVelX[1],4);
			vector float sr_vx = vec_sld(vZero, vVelX[0], 12);
			
			//Sum everything!!!
			tmp = vec_add(sl_p, sr_p);
			tmp = vec_add(tmp, vPressureN[0]);
			tmp = vec_add(tmp, vPressureP[0]);
			tmp = vec_sub(tmp, sl_vx);
			tmp = vec_add(tmp, sr_vx);
			tmp = vec_sub(tmp, vVelYN[0]);
			tmp = vec_add(tmp, vVelYP[0]);
			
			vPressure[0] = vec_madd(tmp, div4, vZero);
			vPressureRow[0] = vPressureRow[1];
		}
		for (x=1; x<w/4-1; x++)
		{
			vector float tmp;
			
			//Compute shifts
			vector float sl_p = vec_sld(vPressure[x], vPressure[x+1],4);
			vector float sr_p = vec_sld(vPressure[x-1], vPressure[x], 12);
			
			vector float sl_vx = vec_sld(vVelX[x], vVelX[x+1],4);
			vector float sr_vx = vec_sld(vVelX[x-1], vVelX[x], 12);
			
			//Sum everything!!!
			tmp = vec_add(sl_p, sr_p);
			tmp = vec_add(tmp, vPressureN[x]);
			tmp = vec_add(tmp, vPressureP[x]);
			tmp = vec_sub(tmp, sl_vx);
			tmp = vec_add(tmp, sr_vx);
			tmp = vec_sub(tmp, vVelYN[x]);
			tmp = vec_add(tmp, vVelYP[x]);
			
			vPressure[x] = vec_madd(tmp, div4, vZero);
		}
		{
			vector float tmp;
			
			//Compute shifts
			vector float sl_p = vec_sld(vPressure[x], vZero,4);
			vector float sr_p = vec_sld(vPressure[x-1], vPressure[x], 12);
			
			vector float sl_vx = vec_sld(vVelX[x], vZero,4);
			vector float sr_vx = vec_sld(vVelX[x-1], vVelX[x], 12);
			
			//Sum everything!!!
			tmp = vec_add(sl_p, sr_p);
			tmp = vec_add(tmp, vPressureN[x]);
			tmp = vec_add(tmp, vPressureP[x]);
			tmp = vec_sub(tmp, sl_vx);
			tmp = vec_add(tmp, sr_vx);
			tmp = vec_sub(tmp, vVelYN[x]);
			tmp = vec_add(tmp, vVelYP[x]);
			
			vPressure[x] = vec_madd(tmp, div4, vZero);
			
			vPressureRow[w-1] = vPressureRow[w-2];
		}
		
#elif defined __SSE3__
		float *vPressureRow = fluidFloatPointer(pressure, y*sy);
		
		__m128 *vPressure = (__m128*)vPressureRow;
		__m128 *vVelX = (__m128*)fluidFloatPointer(velX, y*sy);
		
		__m128 *vPressureN = (__m128*)fluidFloatPointer(pressure, (y+1)*sy);
		__m128 *vVelYN = (__m128*)fluidFloatPointer(velY, (y+1)*sy);
		
		__m128 *vPressureP = (__m128*)fluidFloatPointer(pressure, (y-1)*sy);
		__m128 *vVelYP = (__m128*)fluidFloatPointer(velY, (y-1)*sy);
		
		__m128 div4 = {1.0f/4.0f, 1.0f/4.0f, 1.0f/4.0f, 1.0f/4.0f};
		
		int x;
		{
			__m128 tmp;
			
			//Compute shifts
			__m128 sl_p = _mm_srli_si128(vPressure[0],4);
			sl_p = _mm_add_ps(sl_p,_mm_slli_si128(vPressure[1],12));
			
			__m128 sr_p = _mm_slli_si128(vPressure[0],4);
			
			__m128 sl_vx = _mm_srli_si128(vVelX[0],4);
			sl_vx = _mm_add_ps(sl_vx,_mm_slli_si128(vVelX[1],12));
			
			__m128 sr_vx = _mm_slli_si128(vVelX[0],4);
			
			//Sum everything!!!
			tmp = _mm_add_ps(sl_p, sr_p);
			tmp = _mm_add_ps(tmp, vPressureN[0]);
			tmp = _mm_add_ps(tmp, vPressureP[0]);
			tmp = _mm_sub_ps(tmp, sl_vx);
			tmp = _mm_add_ps(tmp, sr_vx);
			tmp = _mm_sub_ps(tmp, vVelYN[0]);
			tmp = _mm_add_ps(tmp, vVelYP[0]);
			
			vPressure[0] = _mm_mul_ps(tmp, div4);
			vPressureRow[0] = vPressureRow[1];
		}
		for (x=1; x<w/4-1; x++)
		{
			__m128 tmp;
			
			//Compute shifts
			__m128 sl_p = _mm_srli_si128(vPressure[x],4);
			sl_p = _mm_add_ps(sl_p,_mm_slli_si128(vPressure[x+1],12));
			
			__m128 sr_p = _mm_slli_si128(vPressure[x],4);
			sr_p = _mm_add_ps(sr_p,_mm_srli_si128(vPressure[x-1],12));
			
			__m128 sl_vx = _mm_srli_si128(vVelX[x],4);
			sl_vx = _mm_add_ps(sl_vx,_mm_slli_si128(vVelX[x+1],12));
			
			__m128 sr_vx = _mm_slli_si128(vVelX[x],4);
			sr_vx = _mm_add_ps(sr_vx,_mm_srli_si128(vVelX[x-1],12));
			
			//Sum everything!!!
			tmp = _mm_add_ps(sl_p, sr_p);
			tmp = _mm_add_ps(tmp, vPressureN[x]);
			tmp = _mm_add_ps(tmp, vPressureP[x]);
			tmp = _mm_sub_ps(tmp, sl_vx);
			tmp = _mm_add_ps(tmp, sr_vx);
			tmp = _mm_sub_ps(tmp, vVelYN[x]);
			tmp = _mm_add_ps(tmp, vVelYP[x]);
			
			vPressure[x] = _mm_mul_ps(tmp, div4);
		}
		{
			__m128 tmp;
			
			//Compute shifts
			__m128 sl_p = _mm_srli_si128(vPressure[x],4);
			
			__m128 sr_p = _mm_slli_si128(vPressure[x],4);
			sr_p = _mm_add_ps(sr_p,_mm_srli_si128(vPressure[x-1],12));
			
			__m128 sl_vx = _mm_srli_si128(vVelX[x],4);
			
			__m128 sr_vx = _mm_slli_si128(vVelX[x],4);
			sr_vx = _mm_add_ps(sr_vx,_mm_srli_si128(vVelX[x-1],12));
			
			//Sum everything!!!
			tmp = _mm_add_ps(sl_p, sr_p);
			tmp = _mm_add_ps(tmp, vPressureN[x]);
			tmp = _mm_add_ps(tmp, vPressureP[x]);
			tmp = _mm_sub_ps(tmp, sl_vx);
			tmp = _mm_add_ps(tmp, sr_vx);
			tmp = _mm_sub_ps(tmp, vVelYN[x]);
			tmp = _mm_add_ps(tmp, vVelYP[x]);
			
			vPressure[x] = _mm_mul_ps(tmp, div4);
			
			vPressureRow[w-1] = vPressureRow[w-2];
		}
		
#else
		float lastPressureX = fluidFloatPointer(pressure,sx + y*sy)[0];
		float lastVelX = fluidFloatPointer(velX, y*sy)[0];
		
		float curPressureX = lastPressureX;
		float curVelX = fluidFloatPointer(velX, sx + y*sy)[0];
		
		fluidFloatPointer(pressure,y*sy)[0] = lastPressureX;
		
		int x;
		int curxy = sx + y*sy;
		for (x=1; x<w-1; x++)
		{
			float nextPressureX = fluidFloatPointer(pressure,curxy + sx)[0];
			float nextVelX = fluidFloatPointer(velX,curxy + sx)[0];
			
			fluidFloatPointer(pressure,curxy)[0] =
				(	  lastPressureX
				 	+ nextPressureX
				 	+ fluidFloatPointer(pressure,curxy - sy)[0]
					+ fluidFloatPointer(pressure,curxy + sy)[0]
				 - 		(  nextVelX
						 - lastVelX
						 + fluidFloatPointer(velY,curxy + sy)[0]
						 - fluidFloatPointer(velY,curxy - sy)[0])) / 4.0f;
			
			lastPressureX = curPressureX;
			curPressureX = nextPressureX;
			
			lastVelX = curVelX;
			curVelX = nextVelX;
			
			curxy += sx;
		}
		
		fluidFloatPointer(pressure,(w-1)*sx + y*sy)[0]
			= fluidFloatPointer(pressure,(w-2)*sx + y*sy)[0];
#endif
	}
}

/** Function to handle pressure while respecting densities
 	- To get zero pressure we clear out values where needed...*/
void fluid_genPressure_dens(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct pressure *p = &mode->pressure;
	
	int w = fieldWidth(p->velX);
	int h = fieldHeight(p->velX);
	
	int sx = fieldStrideX(p->velX);
	int sy = fieldStrideY(p->velY);
	
	float *velX = fieldData(p->velX);
	float *velY = fieldData(p->velY);
	
	float *pressure = fieldData(p->pressure);
	
	
	int dx = fieldStrideX(p->density);
	int dy = fieldStrideY(p->density);
	float *density = fieldData(p->density);
	
	if (y == 0)
	{
		int x;
		for (x=0; x<w; x++)
		{
			fluidFloatPointer(pressure,x*sx)[0] = fluidFloatPointer(pressure,x*sx + sy)[0];
		}
	}
	else if (y == h-1)
	{
		int x;
		for (x=0; x<w; x++)
		{
			fluidFloatPointer(pressure,x*sx + y*sy)[0] =
			fluidFloatPointer(pressure,x*sx + (y-1)*sy)[0];
		}
	}
	else
	{
		float lastPressureX = fluidFloatPointer(pressure,sx + y*sy)[0];
		float lastVelX = fluidFloatPointer(velX, y*sy)[0];
		
		float curPressureX = lastPressureX;
		float curVelX = fluidFloatPointer(velX, sx + y*sy)[0];
		
		fluidFloatPointer(pressure,y*sy)[0] = lastPressureX;
		
		int x;
		int curxy = sx + y*sy;
		for (x=1; x<w-1; x++)
		{
			float nextPressureX = fluidFloatPointer(pressure,curxy + sx)[0];
			float nextVelX = fluidFloatPointer(velX,curxy + sx)[0];
		
//			if (fluidFloatPointer(density,dx*x + dy*y)[3] > 0)
			{
				fluidFloatPointer(pressure,curxy)[0] =
						(	  lastPressureX
							 + nextPressureX
							 + fluidFloatPointer(pressure,curxy - sy)[0]
							 + fluidFloatPointer(pressure,curxy + sy)[0]
							 - 		fluidFloatPointer(density,dx*x + dy*y)[3]*
						 			(  nextVelX
									 - lastVelX
									 + fluidFloatPointer(velY,curxy + sy)[0]
									 - fluidFloatPointer(velY,curxy - sy)[0])) / 4.0f;
			}
			
			lastPressureX = curPressureX;
			curPressureX = nextPressureX;
			
			lastVelX = curVelX;
			curVelX = nextVelX;
			
			curxy += sx;
		}
		
		fluidFloatPointer(pressure,(w-1)*sx + y*sy)[0]
		= fluidFloatPointer(pressure,(w-2)*sx + y*sy)[0];
	}
}

void fluid_genPressure_densfix(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct pressure *p = &mode->pressure;
	
	
	int dx = fieldStrideX(p->density);
	int dy = fieldStrideY(p->density);
	float *density = fieldData(p->density);
	
	int w = fieldWidth(p->density);
	
	int x;
	for (x=0; x<w; x++)
	{
		float *ptr = fluidFloatPointer(density,dx*x + dy*y);
		if (ptr[3] > 0.9f)
			ptr[3] = 1.0f;
		else if (ptr[3] < 0.1f)
			ptr[3] = 0.0f;
	}
}

/** Applies the results of the pressure */
void fluid_applyPressure(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct pressure *p = &mode->pressure;
	
	int w = fieldWidth(p->velX);
	int h = fieldHeight(p->velX);
	
	int sx = fieldStrideX(p->velX);
	int sy = fieldStrideY(p->velY);
	
	float *velX = fieldData(p->velX);
	float *velY = fieldData(p->velY);
	
	float *pressure = fieldData(p->pressure);
	
	if (y == 0)
	{
	}
	else if (y == h-1)
	{
	}
	else
	{		
		int x;
		for (x=1; x<w-1; x++)
		{
			*fluidFloatPointer(velX,x*sx + y*sy)
				-= *fluidFloatPointer(pressure,(x+1)*sx+y*sy)
					- *fluidFloatPointer(pressure,(x-1)*sx+y*sy);
			
			
			*fluidFloatPointer(velX,x*sx + y*sy)
				= fluidClamp(*fluidFloatPointer(velX,x*sx + y*sy),-9,9);
			
			
		}
		for (x=1; x<w-1; x++)
		{
			*fluidFloatPointer(velY,x*sx + y*sy)
				-= *fluidFloatPointer(pressure,x*sx+(y+1)*sy)
					- *fluidFloatPointer(pressure,x*sx+(y-1)*sy);
			
			*fluidFloatPointer(velY,x*sx + y*sy)
				= fluidClamp(*fluidFloatPointer(velY,x*sx + y*sy),-9,9);
		}
	}
}
