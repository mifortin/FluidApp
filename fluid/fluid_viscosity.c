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

//#undef __APPLE_ALTIVEC__

#include "fluid_macros_2.h"
#include "fluid_cpu.h"
#include <stdio.h>

#include <stdio.h>

void fluid_viscosity(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct viscosity *v = &mode->viscosity;
	
	int w = fieldWidth(v->velX);
	int h = fieldHeight(v->velX);
	
#ifdef __APPLE_ALTIVEC__
#else
	int sx = fieldStrideX(v->velX);
#endif
	int sy = fieldStrideY(v->velY);
	
	float *velX = fieldData(v->velX);
	float *velY = fieldData(v->velY);
	
	float alpha = v->alpha;
	float beta = v->beta;
	
	if (y == 0)
	{
#ifdef __APPLE_ALTIVEC__
		vector float *vVelX = (vector float*)fluidFloatPointer(velX, 0*sy);
		vector float *vVelXP = (vector float*)fluidFloatPointer(velX, 1*sy);
		
		vector float *vVelY = (vector float*)fluidFloatPointer(velY, 0*sy);
		vector float *vVelYP = (vector float*)fluidFloatPointer(velY, 1*sy);
		
		vector bool int vSignBig = {0x80000000,0x80000000,0x80000000,0x80000000};
		
		int x;
		w/=4;
		for (x=0; x<w; x++)
		{
			vVelX[x] = vec_xor(vSignBig,vVelXP[x]);
		}
		for (x=0; x<w; x++)
		{
			vVelY[x] = vec_xor(vSignBig,vVelYP[x]);
		}
#else
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
#endif
	}
	else if (y == h-1)
	{
#ifdef __APPLE_ALTIVEC__
		vector float *vVelX = (vector float*)fluidFloatPointer(velX, y*sy);
		vector float *vVelXP = (vector float*)fluidFloatPointer(velX, (y-1)*sy);
		
		vector float *vVelY = (vector float*)fluidFloatPointer(velY, y*sy);
		vector float *vVelYP = (vector float*)fluidFloatPointer(velY, (y-1)*sy);
		
		vector bool int vSignBig = {0x80000000,0x80000000,0x80000000,0x80000000};
		
		int x;
		w/=4;
		for (x=0; x<w; x++)
		{
			vVelX[x] = vec_xor(vSignBig,vVelXP[x]);
		}
		for (x=0; x<w; x++)
		{
			vVelY[x] = vec_xor(vSignBig,vVelYP[x]);
		}
#else
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
#endif
	}
	else
	{
		
#ifdef __APPLE_ALTIVEC__
//#warning "Using ALTIVEC for Viscosity"
		vector float vAlpha = {alpha, alpha, alpha, alpha};
		vector float vBeta = {beta, beta, beta, beta};
		
		float *velXRow = fluidFloatPointer(velX,y*sy);
		float *velYRow = fluidFloatPointer(velY,y*sy);
		
		vector float *vVelX = (vector float*)velXRow;
		vector float *vVelY = (vector float*)velYRow;
		
		vector float *vVelXN = (vector float*)fluidFloatPointer(velX,(y+1)*sy);
		vector float *vVelYN = (vector float*)fluidFloatPointer(velY,(y+1)*sy);
		
		vector float *vVelXP = (vector float*)fluidFloatPointer(velX,(y-1)*sy);
		vector float *vVelYP = (vector float*)fluidFloatPointer(velY,(y-1)*sy);
		
		vector float vZero = {0,0,0,0};
		
		vec_dstst(vVelX, 0x01000001, 0);
		vec_dst(vVelXN, 0x01000001, 1);
		vec_dst(vVelXP, 0x01000001, 2);
		
		int x;
		{
			vector float tmp;
			tmp = vec_madd(vVelX[0], vAlpha, vVelXN[0]);
			tmp = vec_add(tmp, vVelXP[0]);
			
			vector float sl = vec_sld(vVelX[0], vVelX[1], 4);
			vector float sr = vec_sld(vZero, vVelX[0], 12);
			
			tmp = vec_add(tmp, vec_add(sl,sr));
			vVelX[0] = vec_madd(tmp, vBeta, vZero);
			
			velXRow[0] = -velXRow[1];
		}
		x=1;
		while (x<w/4-13)
		{
			//Out of order dispatch groups
			vector float tmp1 = vec_madd(vVelX[x], vAlpha, vVelXN[x]);
			vector float tmp2 = vec_add(vVelXP[x], tmp1);
			
			vector float tmp1_2 = vec_madd(vVelX[x+1], vAlpha, vVelXN[x+1]);
			vector float tmp2_2 = vec_add(vVelXP[x+1], tmp1_2);
			
			vector float tmp1_3 = vec_madd(vVelX[x+2], vAlpha, vVelXN[x+2]);
			vector float tmp2_3 = vec_add(vVelXP[x+2], tmp1_3);
			
			vector float tmp1_4 = vec_madd(vVelX[x+3], vAlpha, vVelXN[x+3]);
			vector float tmp2_4 = vec_add(vVelXP[x+3], tmp1_4);
			
			//In order dispatch groups
			
			vector float sl = vec_sld(vVelX[x], vVelX[x+1], 4);
			vector float sr = vec_sld(vVelX[x-1], vVelX[x], 12);
			
			vector float sl_2 = vec_sld(vVelX[x+1], vVelX[x+2], 4);
			vector float sr_2 = vec_sld(vVelX[x], vVelX[x+1], 12);
			
			vector float sl_3 = vec_sld(vVelX[x+2], vVelX[x+3], 4);
			vector float sr_3 = vec_sld(vVelX[x+1], vVelX[x+2], 12);
			
			vector float sl_4 = vec_sld(vVelX[x+3], vVelX[x+4], 4);
			vector float sr_4 = vec_sld(vVelX[x+2], vVelX[x+3], 12);
			
			
			vector float tmp3 = vec_add(sl,sr);
			tmp1 = vec_add(tmp3, tmp2);
			vVelX[x] = vec_madd(tmp1, vBeta, vZero);
			
			
			
			vector float tmp3_2 = vec_add(sl_2,sr_2);
			tmp1_2 = vec_add(tmp3_2, tmp2_2);
			vVelX[x+1] = vec_madd(tmp1_2, vBeta, vZero);
			
			
			
			
			vector float tmp3_3 = vec_add(sl_3,sr_3);
			tmp1_3 = vec_add(tmp3_3, tmp2_3);
			vVelX[x+2] = vec_madd(tmp1_3, vBeta, vZero);
			
			
			
			vector float tmp3_4 = vec_add(sl_4,sr_4);
			tmp1_4 = vec_add(tmp3_4, tmp2_4);
			vVelX[x+3] = vec_madd(tmp1_4, vBeta, vZero);
			
			x+=4;
		}
		while (x<w/4-1)
		{			
			vector float sl = vec_sld(vVelX[x], vVelX[x+1], 4);
			vector float sr = vec_sld(vVelX[x-1], vVelX[x], 12);
			
			vector float tmp1 = vec_madd(vVelX[x], vAlpha, vVelXN[x]);
			vector float tmp3 = vec_add(sl,sr);
			vector float tmp2 = vec_add(vVelXP[x], tmp1);
			tmp1 = vec_add(tmp3, tmp2);
			vVelX[x] = vec_madd(tmp1, vBeta, vZero);
			x++;
		}
		{
			vec_dstst(vVelY, 0x01000001, 0);
			vec_dst(vVelYN, 0x01000001, 1);
			vec_dst(vVelYP, 0x01000001, 2);
			
			vector float tmp;
			tmp = vec_madd(vVelX[0], vAlpha, vVelXN[0]);
			tmp = vec_add(tmp, vVelXP[0]);
			
			vector float sl = vec_sld(vVelX[x], vZero, 4);
			vector float sr = vec_sld(vVelX[x-1], vVelX[x], 12);
			
			tmp = vec_add(tmp, vec_add(sl,sr));
			vVelX[x] = vec_madd(tmp, vBeta, vZero);
			
			velXRow[w-1] = -velXRow[w-2];
		}
		
		
		{
			vector float tmp;
			tmp = vec_madd(vVelY[0], vAlpha, vVelYN[0]);
			tmp = vec_add(tmp, vVelYP[0]);
			
			vector float sl = vec_sld(vVelY[0], vVelY[1], 4);
			vector float sr = vec_sld(vZero, vVelY[0], 12);
			
			tmp = vec_add(tmp, vec_add(sl,sr));
			vVelY[0] = vec_madd(tmp, vBeta, vZero);
			
			velYRow[0] = -velYRow[1];
		}
		x=1;
		while (x<w/4-13)
		{
			//Move shifts here for parallel execution on proc
			vector float sl = vec_sld(vVelY[x], vVelY[x+1], 4);
			vector float sr = vec_sld(vVelY[x-1], vVelY[x], 12);
			
			vector float sl_2 = vec_sld(vVelY[x+1], vVelY[x+2], 4);
			vector float sr_2 = vec_sld(vVelY[x], vVelY[x+1], 12);
			
			vector float sl_3 = vec_sld(vVelY[x+2], vVelY[x+3], 4);
			vector float sr_3 = vec_sld(vVelY[x+1], vVelY[x+2], 12);
			
			vector float sl_4 = vec_sld(vVelY[x+3], vVelY[x+4], 4);
			vector float sr_4 = vec_sld(vVelY[x+2], vVelY[x+3], 12);
			
			//Rest of code...
			
			
			vector float tmp1 = vec_madd(vVelY[x], vAlpha, vVelYN[x]);
			vector float tmp3 = vec_add(sl,sr);
			vector float tmp2 = vec_add(tmp1, vVelYP[x]);
			tmp1 = vec_add(tmp3, tmp2);
			vVelY[x] = vec_madd(tmp1, vBeta, vZero);
			
			
			
			vector float tmp1_2 = vec_madd(vVelY[x+1], vAlpha, vVelYN[x+1]);
			vector float tmp3_2 = vec_add(sl_2,sr_2);
			vector float tmp2_2 = vec_add(tmp1_2, vVelYP[x+1]);
			tmp1_2 = vec_add(tmp3_2, tmp2_2);
			vVelY[x+1] = vec_madd(tmp1_2, vBeta, vZero);
			
			
			
			vector float tmp1_3 = vec_madd(vVelY[x+2], vAlpha, vVelYN[x+2]);
			vector float tmp3_3 = vec_add(sl_3,sr_3);
			vector float tmp2_3 = vec_add(tmp1_3, vVelYP[x+2]);
			tmp1_3 = vec_add(tmp3_3, tmp2_3);
			vVelY[x+2] = vec_madd(tmp1_3, vBeta, vZero);
			
			
			
			vector float tmp1_4 = vec_madd(vVelY[x+3], vAlpha, vVelYN[x+3]);
			vector float tmp3_4 = vec_add(sl_4,sr_4);
			vector float tmp2_4 = vec_add(tmp1_4, vVelYP[x+3]);
			tmp1_4 = vec_add(tmp3_4, tmp2_4);
			vVelY[x+3] = vec_madd(tmp1_4, vBeta, vZero);
			
			x+=4;
		}
		while (x<w/4-1)
		{
			vector float sl = vec_sld(vVelY[x], vVelY[x+1], 4);
			vector float sr = vec_sld(vVelY[x-1], vVelY[x], 12);
			
			vector float tmp1 = vec_madd(vVelY[x], vAlpha, vVelYN[x]);
			vector float tmp3 = vec_add(sl,sr);
			vector float tmp2 = vec_add(tmp1, vVelYP[x]);
			tmp1 = vec_add(tmp3, tmp2);
			vVelY[x] = vec_madd(tmp1, vBeta, vZero);
			x++;
		}
		{
			vector float tmp;
			tmp = vec_madd(vVelY[x], vAlpha, vVelYN[x]);
			tmp = vec_add(tmp, vVelYP[x]);
			
			vector float sl = vec_sld(vVelY[x], vZero, 4);
			vector float sr = vec_sld(vVelY[x-1], vVelY[x], 12);
			
			tmp = vec_add(tmp, vec_add(sl,sr));
			vVelY[x] = vec_madd(tmp, vBeta, vZero);
			
			velYRow[w-1] = -velYRow[w-2];
		}
		
#elif defined __SSE3__
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
		
#define SSE_VISC_X_PRE(n) \
		__m128 sl ## n = _mm_srli_sf128(vVelX[x+n],4);						\
		__m128 sr ## n = _mm_slli_sf128(vVelX[x+n],4);						\
		sl ## n = _mm_add_ps(sl ## n, _mm_slli_sf128(vVelX[x+1+n],12));		\
		sr ## n = _mm_add_ps(sr ## n, _mm_srli_sf128(vVelX[x-1+n],12));
		
#define SSE_VISC_X_POST(n) \
		__m128 tmp1 ## n = _mm_mul_ps(vVelX[x+n], vAlpha);					\
		__m128 tmp2 ## n = _mm_add_ps(vVelXP[x+n], vVelXN[x+n]);			\
		__m128 tmp3 ## n = _mm_add_ps(sl ## n, sr ## n);					\
																			\
		tmp1 ## n = _mm_add_ps(tmp1 ## n, tmp2 ## n);						\
		tmp1 ## n = _mm_add_ps(tmp1 ## n, tmp3 ## n);						\
		vVelX[x+n] = _mm_mul_ps(tmp1 ## n, vBeta);
		
		
		
#define SSE_VISC_Y_PRE(n) \
		__m128 sl ## n = _mm_srli_sf128(vVelY[x+n],4);						\
		__m128 sr ## n = _mm_slli_sf128(vVelY[x+n],4);						\
		sl ## n = _mm_add_ps(sl ## n, _mm_slli_sf128(vVelY[x+1+n],12));		\
		sr ## n = _mm_add_ps(sr ## n, _mm_srli_sf128(vVelY[x-1+n],12));
		
#define SSE_VISC_Y_POST(n) \
		__m128 tmp1 ## n = _mm_mul_ps(vVelY[x+n], vAlpha);					\
		__m128 tmp2 ## n = _mm_add_ps(vVelYP[x+n], vVelYN[x+n]);			\
		__m128 tmp3 ## n = _mm_add_ps(sl ## n, sr ## n);					\
																			\
		tmp1 ## n = _mm_add_ps(tmp1 ## n, tmp2 ## n);						\
		tmp1 ## n = _mm_add_ps(tmp1 ## n, tmp3 ## n);						\
		vVelY[x+n] = _mm_mul_ps(tmp1 ## n, vBeta);
		
		int x;
		{
			__m128 tmp;
			tmp = _mm_mul_ps(vVelX[0], vAlpha);
			tmp = _mm_add_ps(tmp, vVelXN[0]);
			tmp = _mm_add_ps(tmp, vVelXP[0]);
			
			//NB: right does left, left does right!  (little endian woes)
			__m128 sl = _mm_srli_sf128(vVelX[0],4);
			__m128 sr = _mm_slli_sf128(vVelX[0],4);
			sl = _mm_add_ps(sl, _mm_slli_sf128(vVelX[1],12));
			
			tmp = _mm_add_ps(tmp, sl);
			tmp = _mm_add_ps(tmp, sr);
			
			vVelX[0] = _mm_mul_ps(tmp, vBeta);
			velXRow[0] = -velXRow[1];
		}
		x=1;
		while(x<w/4-9)
		{
			SSE_VISC_X_PRE(0);
			SSE_VISC_X_PRE(1);
			SSE_VISC_X_PRE(2);
			
			SSE_VISC_X_POST(0);
			SSE_VISC_X_POST(1);
			SSE_VISC_X_POST(2);
			
			x+=3;
		}
		while(x<w/4-1)
		{
			SSE_VISC_X_PRE(0);
			SSE_VISC_X_POST(0);
			
			x++;
		}
		{
			__m128 tmp;
			tmp = _mm_mul_ps(vVelX[x], vAlpha);
			tmp = _mm_add_ps(tmp, vVelXN[x]);
			tmp = _mm_add_ps(tmp, vVelXP[x]);
			
			__m128 sl = _mm_srli_sf128(vVelX[x],4);
			__m128 sr = _mm_slli_sf128(vVelX[x],4);
			sr = _mm_add_ps(sr, _mm_srli_sf128(vVelX[x-1],12));
			
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
			
			__m128 sl = _mm_srli_sf128(vVelY[0],4);
			__m128 sr = _mm_slli_sf128(vVelY[0],4);
			sl = _mm_add_ps(sl, _mm_slli_sf128(vVelY[1],12));
			
			tmp = _mm_add_ps(tmp, sl);
			tmp = _mm_add_ps(tmp, sr);
			
			vVelY[0] = _mm_mul_ps(tmp, vBeta);
			velYRow[0] = -velYRow[1];
		}
		x=1;
		while(x<w/4-9)
		{
			SSE_VISC_Y_PRE(0);
			SSE_VISC_Y_PRE(1);
			SSE_VISC_Y_PRE(2);
			
			SSE_VISC_Y_POST(0);
			SSE_VISC_Y_POST(1);
			SSE_VISC_Y_POST(2);
			
			x+=3;
		}
		while(x<w/4-1)
		{
			SSE_VISC_Y_PRE(0);
			SSE_VISC_Y_POST(0);
			
			x++;
		}
		{
			__m128 tmp;
			tmp = _mm_mul_ps(vVelY[x], vAlpha);
			tmp = _mm_add_ps(tmp, vVelYN[x]);
			tmp = _mm_add_ps(tmp, vVelYP[x]);
			
			__m128 sl = _mm_srli_sf128(vVelY[x],4);
			__m128 sr = _mm_slli_sf128(vVelY[x],4);
			sr = _mm_add_ps(sr, _mm_srli_sf128(vVelY[x-1],12));
			
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

