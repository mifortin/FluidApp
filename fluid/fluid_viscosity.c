/*
 *  fluid_viscosity.c
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
#include <stdio.h>

#include <stdio.h>

void fluid_viscosity(fluid *in_f, int y, pvt_fluidMode *mode)
{
#ifdef __APPLE_ALTIVEC__
#endif
	struct viscosity *v = &mode->viscosity;
	
	int w = fieldWidth(v->velX);
	int h = fieldHeight(v->velX);
	
	int sx = fieldStrideX(v->velX);
	int sy = fieldStrideY(v->velY);
	
	float *velX = fieldData(v->velX);
	float *velY = fieldData(v->velY);
	
	float alpha = v->alpha;
	float beta = v->beta;
	
	if (y == 0)
	{
		int x;
		for (x=0; x<w; x++)
		{
			*fluidFloatPointer(velX,x*sx + y*sy)
							= - *fluidFloatPointer(velX,x*sx + (y+1)*sy);
		}
		for (x=0; x<w; x++)
		{
			*fluidFloatPointer(velY,x*sx + y*sy)
							= - *fluidFloatPointer(velY,x*sx + (y+1)*sy);
		}
	}
	else if (y == h-1)
	{
		int x;
		for (x=0; x<w; x++)
		{			
			*fluidFloatPointer(velX,x*sx + y*sy)
							= - *fluidFloatPointer(velX,x*sx + (y-1)*sy);
		}
		for (x=0; x<w; x++)
		{
			*fluidFloatPointer(velY,x*sx + y*sy)
							= - *fluidFloatPointer(velY,x*sx + (y-1)*sy);
		}
	}
	else
	{
#ifdef __SSE3__
		__m128 vAlpha = {alpha, alpha, alpha, alpha};
		__m128 vBeta = {beta, beta, beta, beta};
		
		float *velXRow = fluidFloatPointer(velX,y*sy);
		float *velYRow = fluidFloatPointer(velY,y*sy);
		
		__m128 *vVelX = (__m128*)velXRow;
		__m128 *vVelY = (__m128*)velYRow;
		
		__m128 *vVelXN = (__m128*)fluidFloatPointer(velX,(y+1)*sy);
		__m128 *vVelYN = (__m128*)fluidFloatPointer(velY,(y+1)*sy);
		
		__m128 *vVelXP = (__m128*)fluidFloatPointer(velX,(y-1)*sy);
		__m128 *vVelYP = (__m128*)fluidFloatPointer(velY,(y-1)*sy);
		
		int x;
		{
			__m128 tmp;
			tmp = _mm_mul_ps(vVelX[0], vAlpha);
			tmp = _mm_add_ps(tmp, vVelXN[0]);
			tmp = _mm_add_ps(tmp, vVelXP[0]);
			
			//NB: right does left, left does right!  (little endian woes)
			__m128 sl = _mm_srli_si128(vVelX[0],4);
			__m128 sr = _mm_slli_si128(vVelX[0],4);
			sl = _mm_add_ps(sl, _mm_slli_si128(vVelX[1],12));
			
			tmp = _mm_add_ps(tmp, sl);
			tmp = _mm_add_ps(tmp, sr);
			
			vVelX[0] = _mm_mul_ps(tmp, vBeta);
			velXRow[0] = -velXRow[1];
		}
		for (x=1; x<w/4-1; x++)
		{
			__m128 tmp;
			tmp = _mm_mul_ps(vVelX[x], vAlpha);
			tmp = _mm_add_ps(tmp, vVelXN[x]);
			tmp = _mm_add_ps(tmp, vVelXP[x]);
			
			__m128 sl = _mm_srli_si128(vVelX[x],4);
			__m128 sr = _mm_slli_si128(vVelX[x],4);
			sl = _mm_add_ps(sl, _mm_slli_si128(vVelX[x+1],12));
			sr = _mm_add_ps(sr, _mm_srli_si128(vVelX[x-1],12));
			
			tmp = _mm_add_ps(tmp, sl);
			tmp = _mm_add_ps(tmp, sr);
			
			vVelX[x] = _mm_mul_ps(tmp, vBeta);
		}
		{
			__m128 tmp;
			tmp = _mm_mul_ps(vVelX[x], vAlpha);
			tmp = _mm_add_ps(tmp, vVelXN[x]);
			tmp = _mm_add_ps(tmp, vVelXP[x]);
			
			__m128 sl = _mm_srli_si128(vVelX[x],4);
			__m128 sr = _mm_slli_si128(vVelX[x],4);
			sr = _mm_add_ps(sr, _mm_srli_si128(vVelX[x-1],12));
			
			tmp = _mm_add_ps(tmp, sl);
			tmp = _mm_add_ps(tmp, sr);
			
			vVelX[x] = _mm_mul_ps(tmp, vBeta);
			velXRow[w-1] = -velXRow[w-2];
		}
		
		
		{
			__m128 tmp;
			tmp = _mm_mul_ps(vVelY[0], vAlpha);
			tmp = _mm_add_ps(tmp, vVelYN[0]);
			tmp = _mm_add_ps(tmp, vVelYP[0]);
			
			__m128 sl = _mm_srli_si128(vVelY[0],4);
			__m128 sr = _mm_slli_si128(vVelY[0],4);
			sl = _mm_add_ps(sl, _mm_slli_si128(vVelY[1],12));
			
			tmp = _mm_add_ps(tmp, sl);
			tmp = _mm_add_ps(tmp, sr);
			
			vVelY[0] = _mm_mul_ps(tmp, vBeta);
			velYRow[0] = 0;//-velYRow[1];
		}
		for (x=1; x<w/4-1; x++)
		{
			__m128 tmp;
			tmp = _mm_mul_ps(vVelY[x], vAlpha);
			tmp = _mm_add_ps(tmp, vVelYN[x]);
			tmp = _mm_add_ps(tmp, vVelYP[x]);
			
			__m128 sl = _mm_srli_si128(vVelY[x],4);
			__m128 sr = _mm_slli_si128(vVelY[x],4);
			sl = _mm_add_ps(sl, _mm_slli_si128(vVelY[x+1],12));
			sr = _mm_add_ps(sr, _mm_srli_si128(vVelY[x-1],12));
			
			tmp = _mm_add_ps(tmp, sl);
			tmp = _mm_add_ps(tmp, sr);
			
			vVelY[x] = _mm_mul_ps(tmp, vBeta);
		}
		{
			__m128 tmp;
			tmp = _mm_mul_ps(vVelY[x], vAlpha);
			tmp = _mm_add_ps(tmp, vVelYN[x]);
			tmp = _mm_add_ps(tmp, vVelYP[x]);
			
			__m128 sl = _mm_srli_si128(vVelY[x],4);
			__m128 sr = _mm_slli_si128(vVelY[x],4);
			sr = _mm_add_ps(sr, _mm_srli_si128(vVelY[x-1],12));
			
			tmp = _mm_add_ps(tmp, sl);
			tmp = _mm_add_ps(tmp, sr);
			
			vVelY[x] = _mm_mul_ps(tmp, vBeta);
			velYRow[w-1] = -velYRow[w-2];
		}
#else
		float curVelX = fluidFloatPointer(velX,sx + y*sy)[0];
		float curVelY = fluidFloatPointer(velY,sx + y*sy)[0];
		
		float nextVelX;
		float nextVelY;
		
		float pVelX = -curVelX;
		float pVelY = -curVelY;
		
		fluidFloatPointer(velX,y*sy)[0] = -curVelX;
		fluidFloatPointer(velY,y*sy)[0] = -curVelY;
		
		int x;
		int curxy = sx + y*sy;
		for (x=1; x<w-1; x++)
		{
			nextVelY = fluidFloatPointer(velY,curxy + sx)[0];
			
			fluidFloatPointer(velY,curxy)[0]
				= (fluidFloatPointer(velY,curxy)[0]*alpha
				   + pVelY
				   + nextVelY
				   + fluidFloatPointer(velY,curxy - sy)[0]
				   + fluidFloatPointer(velY,curxy + sy)[0]) * beta;
			
			pVelY = curVelY;
			
			curVelY = nextVelY;
			
			curxy += sx;
		}
		
		curxy = sx + y*sy;
		for (x=1; x<w-1; x++)
		{
			nextVelX = fluidFloatPointer(velX,curxy + sx)[0];
			
			fluidFloatPointer(velX,curxy)[0]
				 = (fluidFloatPointer(velX,curxy)[0]*alpha
					 + pVelX
					 + nextVelX
					 + fluidFloatPointer(velX,curxy - sy)[0]
					 + fluidFloatPointer(velX,curxy + sy)[0]) * beta;
			
			pVelX = curVelX;
			
			curVelX = nextVelX;
			
			curxy += sx;
		}
		
		fluidFloatPointer(velX,(w-1)*sx + y*sy)[0]
						= -pVelX;
		fluidFloatPointer(velY,(w-1)*sx + y*sy)[0]
						= -pVelY;
#endif
	}
}

