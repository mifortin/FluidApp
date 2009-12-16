/*
 *  fluid_advection_fwd2.c
 *  FluidApp
 *
 *		This advection scheme attempts to accurately follow the characteristics
 *		of a given particle in order to improve advection.   The theory being
 *		that this will run extremely fast given it's deterministic.
 *
 */

#include "fluid_macros_2.h"
#include "fluid_cpu.h"
#include <math.h>
#include <stdio.h>

#ifdef __SSE3__
#include <xmmintrin.h>
#include <emmintrin.h>
#include <pmmintrin.h>
#include <tmmintrin.h>
//#undef __SSE3__
#endif


void fluid_advection_fwd_generate_repos(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct advection_stam_velocity *d = &mode->advection_stam_velocity;
	
	int w = fieldWidth(d->srcVelX);
	
	float *velX		= fieldData(d->srcVelX);
	float *velY		= fieldData(d->srcVelY);
	
	float *destX	= fieldData(d->dstVelX);
	float *destY	= fieldData(d->dstVelY);
	
	int sY = fieldStrideY(d->srcVelY);
	
	float timestep = -d->timestep / 9.0f;
	
	float fy = (float)y;
	int x;
	
#ifdef __APPLE_ALTIVEC__
	
	vector float *vVelX = (vector float*)fluidFloatPointer(velX, y*sY);
	vector float *vVelY = (vector float*)fluidFloatPointer(velY, y*sY);
	
	vector float *vDX = (vector float*)fluidFloatPointer(destX, y*sY);
	vector float *vDY = (vector float*)fluidFloatPointer(destY, y*sY);
	
	vector float fvx = {0,1,2,3};
	vector float fvx2 = {4,5,6,7};
	vector float fvx3 = {8,9,10,11};
	vector float fvx4 = {12,13,14,15};
	vector float fvy = {fy,fy,fy,fy};
	vector float v4 = {4,4,4,4};
	vector float v16 = {16,16,16,16};
	
	vector float vT = {timestep, timestep, timestep, timestep};
	
	w/=4;
	
	x=0;
	while (x<w-3)
	{
		vector float r1 = vec_madd(vT, vVelX[x], fvx);
		vector float r2 = vec_madd(vT, vVelX[x+1], fvx2);
		vector float r3 = vec_madd(vT, vVelX[x+2], fvx3);
		vector float r4 = vec_madd(vT, vVelX[x+3], fvx4);
		
		fvx = vec_add(fvx, v16);
		fvx2 = vec_add(fvx2, v16);
		fvx3 = vec_add(fvx3, v16);
		fvx4 = vec_add(fvx4, v16);
		
		vDX[x] = r1;
		vDX[x+1] = r2;
		vDX[x+2] = r3;
		vDX[x+3] = r4;
		
		x+=4;
	}
	while (x<w)
	{
		vector float r1 = vec_madd(vT, vVelX[x], fvx);
		
		fvx = vec_add(fvx, v4);
		
		vDX[x] = r1;
		
		x++;
	}
	
	x=0;
	while (x<w-3)
	{
		vDY[x] = vec_madd(vT, vVelY[x], fvy);
		vDY[x+1] = vec_madd(vT, vVelY[x+1], fvy);
		vDY[x+2] = vec_madd(vT, vVelY[x+2], fvy);
		vDY[x+3] = vec_madd(vT, vVelY[x+3], fvy);
		
		x+=4;
	}
	while (x<w)
	{
		vDY[x] = vec_madd(vT, vVelY[x], fvy);
		
		x++;
	}
#else
	int sX = fieldStrideX(d->srcVelY);
	float fx;
	for (x=0,fx=0; x<w; x++,fx++)
	{
		// Note (x+1 - (x-1))/2 = 2/2 = 1
		// Note (x-x)/2 = 0
		fluidFloatPointer(destX, x*sX + y*sY)[0] =
					fx
					+ timestep * fluidFloatPointer(velX,x*sX + y*sY)[0];
	}
	
	for (x=0; x<w; x++)
	{
		fluidFloatPointer(destY, x*sX + y*sY)[0] =
					fy
					+ timestep * fluidFloatPointer(velY, x*sX + y*sY)[0];
	}
#endif
}



void fluid_advection_fwd_repos(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct advection_stam_velocity *d = &mode->advection_stam_velocity;
	
	int w = fieldWidth(d->srcVelX);
	int h = fieldHeight(d->srcVelY);
	
	float *velX		= fieldData(d->srcVelX);
	float *velY		= fieldData(d->srcVelY);
	
	float *destX	= fieldData(d->dstVelX);
	float *destY	= fieldData(d->dstVelY);
	
	int sY = fieldStrideY(d->srcVelY);
	int sX = fieldStrideX(d->srcVelY);
	
	float timestep = -d->timestep / 18.0f;
	
	if (y==0 || y == h-1)
	{
		if (d->clamp)
		{
			float w2 = (float)w-2;
			float h2 = (float)h-2;
			int x;
			float fx;
			x=0;
			fluidFloatPointer(destX, x*sX + y*sY)[0] = 0;
			for (x=1,fx=1; x<w-1; x++,fx++)
			{
				float vx = fluidFloatPointer(velX, x*sX + y*sY)[0];
				
				float dLeft = fluidFloatPointer(destX, (x-1)*sX + y*sY)[0];
				float dRight = fluidFloatPointer(destX, (x+1)*sX + y*sY)[0];
				
				// Note (x+1 - (x-1))/2 = 2/2 = 1
				// Note (x-x)/2 = 0
				fluidFloatPointer(destX, x*sX + y*sY)[0] +=
						timestep * vx * (dRight - dLeft);
				
				fluidFloatPointer(destX, x*sX + y*sY)[0] = 
					fminf(fmaxf(fluidFloatPointer(destX, x*sX + y*sY)[0],0),w2);
				
				
				fluidFloatPointer(destX, x*sX + y*sY)[0] = 
					fminf(fmaxf(fluidFloatPointer(destX, x*sX + y*sY)[0],fx-9),fx+9);
			}
			fluidFloatPointer(destX, x*sX + y*sY)[0] = w-2;
			
			x=0;
			fluidFloatPointer(destY, x*sX + y*sY)[0] = y==0?0:h-2;
			float fy = (float)y;
			for (x=1; x<w-1; x++)
			{
				float vx = fluidFloatPointer(velX, x*sX + y*sY)[0];
				
				float dLeft = fluidFloatPointer(destY, (x-1)*sX + y*sY)[0];
				float dRight = fluidFloatPointer(destY, (x+1)*sX + y*sY)[0];
				
				// Note (x+1 - (x-1))/2 = 2/2 = 1
				// Note (x-x)/2 = 0
				fluidFloatPointer(destY, x*sX + y*sY)[0] +=
						timestep * vx * (dRight - dLeft);
				
				fluidFloatPointer(destY, x*sX + y*sY)[0] = 
					fminf(fmaxf(fluidFloatPointer(destY, x*sX + y*sY)[0],0),h2);
				
				fluidFloatPointer(destY, x*sX + y*sY)[0] = 
					fminf(fmaxf(fluidFloatPointer(destY, x*sX + y*sY)[0],fy-9),fy+9);
			}
			fluidFloatPointer(destY, x*sX + y*sY)[0] = y==0?0:h-2;
		}
		else
		{
			int x;
			fluidFloatPointer(destX, y*sY)[0] = 0;
			for (x=1; x<w-1; x++)
			{
				float vx = fluidFloatPointer(velX, x*sX + y*sY)[0];
				
				float dLeft = fluidFloatPointer(destX, (x-1)*sX + y*sY)[0];
				float dRight = fluidFloatPointer(destX, (x+1)*sX + y*sY)[0];
				
				// Note (x+1 - (x-1))/2 = 2/2 = 1
				// Note (x-x)/2 = 0
				fluidFloatPointer(destX, x*sX + y*sY)[0] +=
						timestep * vx * (dRight - dLeft);
			}
			fluidFloatPointer(destX, x*sX + y*sY)[0] = (float)(w-1);
			
			for (x=1; x<w-1; x++)
			{
				float vx = fluidFloatPointer(velX, x*sX + y*sY)[0];
				
				float dLeft = fluidFloatPointer(destY, (x-1)*sX + y*sY)[0];
				float dRight = fluidFloatPointer(destY, (x+1)*sX + y*sY)[0];
				
				// Note (x+1 - (x-1))/2 = 2/2 = 1
				// Note (x-x)/2 = 0
				fluidFloatPointer(destY, x*sX + y*sY)[0] +=
					timestep * vx * (dRight - dLeft);
			}
		}
	}
	else
	{
#ifdef __APPLE_ALTIVEC__
		vector float *vVelX = (vector float*)fluidFloatPointer(velX, y*sY);
		vector float *vVelY = (vector float*)fluidFloatPointer(velY, y*sY);
		
		vector float *vDX = (vector float*)fluidFloatPointer(destX, y*sY);
		vector float *vDY = (vector float*)fluidFloatPointer(destY, y*sY);
		
		vector float *vDXN = (vector float*)fluidFloatPointer(destX, (y+1)*sY);
		vector float *vDYN = (vector float*)fluidFloatPointer(destY, (y+1)*sY);
		
		vector float *vDXP = (vector float*)fluidFloatPointer(destX, (y-1)*sY);
		vector float *vDYP = (vector float*)fluidFloatPointer(destY, (y-1)*sY);
		
		vector float vZero = {0,0,0,0};
		vector float vPN = {0,0,0,0};
		
		vector float v9 = {9,9,9,9};
		
		float fw = (float)w-2.0f;
		float fh = (float)h-2.0f;
		
		vector float vNNX = {fw,fw,fw,fw};
		vector float vNNY = {fh,fh,fh,fh};
		
		vector float vT = {timestep, timestep, timestep, timestep};
		
#define ADVECT_X_PRE(n)	\
		vector float sl_vx ## n = vec_sld(vDX[x + n], vDX[x+1 + n],4);\
		vector float sr_vx ## n = vec_sld(vDX[x-1 + n], vDX[x + n], 12);
		
#define ADVECT_X_PRE_0(n)	\
		vector float sl_vx ## n = vec_sld(vDX[x + n], vDX[x+1 + n],4);\
		vector float sr_vx ## n = vec_sld(vZero, vDX[x + n], 12);
		
#define ADVECT_X_PRE_N(n)	\
		vector float sl_vx ## n = vec_sld(vDX[x + n], vZero,4);\
		vector float sr_vx ## n = vec_sld(vDX[x-1 + n], vDX[x + n], 12);
		
#define ADVECT_Y_PRE(n)	\
		vector float sl_vy ## n = vec_sld(vDY[x + n], vDY[x+1 + n],4);	\
		vector float sr_vy ## n = vec_sld(vDY[x-1 + n], vDY[x + n], 12);
		
#define ADVECT_Y_PRE_0(n)	\
		vector float sl_vy ## n = vec_sld(vDY[x + n], vDY[x+1 + n],4);	\
		vector float sr_vy ## n = vec_sld(vZero, vDY[x + n], 12);
		
#define ADVECT_Y_PRE_N(n)	\
		vector float sl_vy ## n = vec_sld(vDY[x + n], vZero,4);	\
		vector float sr_vy ## n = vec_sld(vDY[x-1 + n], vDY[x + n], 12);
		
#define ADVECT_X_POST(n)												\
		vector float postX ## n = vec_madd(vT, vec_madd(vVelX[x+n],		\
							vec_sub(sl_vx ## n, sr_vx ## n),			\
								vec_madd(vVelY[x+n],					\
									vec_sub(vDXN[x+n], vDXP[x+n]),	\
										 vZero)),						\
							vDX[x+n]);
		
#define ADVECT_X_ASSIGN(n)											\
		postX ## n = vec_min(vec_max(postX ## n, vPN), vNNX); \
		vDX[x+n] = vec_min(vec_max(postX ## n, vec_sub(vX ## n, v9)), vec_add(vX##n,v9));
		
#define ADVECT_X_ASSIGN_SIMPLE(n)											\
		vDX[x+n] = (postX ## n);
		
#define ADVECT_Y_POST(n)											\
		vector float postY ## n = vec_madd(vT, vec_madd(vVelX[x+n],	\
						vec_sub(sl_vy ## n, sr_vy ## n),			\
							vec_madd(vVelY[x+n],					\
								vec_sub(vDYN[x+n], vDYP[x+n]),	\
									vZero)),						\
						vDY[x+n]);
		
#define ADVECT_Y_ASSIGN(n)										\
		vDY[x+n] = vec_min(vec_max(postY ## n, vPN), vNNY);
		
#define ADVECT_Y_ASSIGN_SIMPLE(n)										\
		vDY[x+n] = (postY ## n);
		
		w = w/4;
		int x;
		if (d->clamp)
		{
			vector float vX0 = {0,1,2,3};
			vector float vX1 = {4,5,6,7};
			vector float vX2 = {8,9,10,11};
			vector float vX3 = {12,13,14,15};
			vector float v4 = {4,4,4,4};
			vector float v16 = {16,16,16,16};
			x = 0;
			{
				ADVECT_X_PRE_0(0);
				ADVECT_X_POST(0);
				ADVECT_X_ASSIGN(0);
				vX0 = vec_add(vX0, v4);
				vX1 = vec_add(vX1, v4);
				vX2 = vec_add(vX2, v4);
				vX3 = vec_add(vX3, v4);
			}
			x=1;
			while (x < w-4)
			{
				ADVECT_X_PRE(0);
				ADVECT_X_PRE(1);
				ADVECT_X_PRE(2);
				ADVECT_X_PRE(3);
				
				ADVECT_X_POST(0);
				ADVECT_X_POST(1);
				ADVECT_X_POST(2);
				ADVECT_X_POST(3);
				
				ADVECT_X_ASSIGN(0);
				ADVECT_X_ASSIGN(1);
				ADVECT_X_ASSIGN(2);
				ADVECT_X_ASSIGN(3);
				
				vX0 = vec_add(vX0, v16);
				vX1 = vec_add(vX1, v16);
				vX2 = vec_add(vX2, v16);
				vX3 = vec_add(vX3, v16);
				
				x+=4;
			}
			while( x<w-1)
			{
				ADVECT_X_PRE(0);
				ADVECT_X_POST(0);
				ADVECT_X_ASSIGN(0);
				vX0 = vec_add(vX0, v4);
				x++;
			}
			{
				ADVECT_X_PRE_N(0);
				ADVECT_X_POST(0);
				ADVECT_X_ASSIGN(0);
			}
			
			
			vector float vY = {(float)y,(float)y,(float)y,(float)y};
			vNNY = vec_min(vNNY, vec_add(vY, v9));
			vPN = vec_max(vPN, vec_sub(vY, v9));
			x=0;
			{
				ADVECT_Y_PRE_0(0);
				ADVECT_Y_POST(0);
				ADVECT_Y_ASSIGN(0);
			}
			x=1;
			while(x<w-4)
			{
				ADVECT_Y_PRE(0);
				ADVECT_Y_PRE(1);
				ADVECT_Y_PRE(2);
				ADVECT_Y_PRE(3);
				
				ADVECT_Y_POST(0);
				ADVECT_Y_POST(1);
				ADVECT_Y_POST(2);
				ADVECT_Y_POST(3);
				
				ADVECT_Y_ASSIGN(0);
				ADVECT_Y_ASSIGN(1);
				ADVECT_Y_ASSIGN(2);
				ADVECT_Y_ASSIGN(3);
				x+=4;
			}
			while(x<w-1)
			{
				ADVECT_Y_PRE(0);
				ADVECT_Y_POST(0);
				ADVECT_Y_ASSIGN(0);
				x++;
			}
			{
				ADVECT_Y_PRE_N(0);
				ADVECT_Y_POST(0);
				ADVECT_Y_ASSIGN(0);
			}
		}
		else
		{
			x=0;
			{
				ADVECT_X_PRE_0(0);
				ADVECT_X_POST(0);
				ADVECT_X_ASSIGN_SIMPLE(0);
			}
			x=1;
			while(x<w-4)
			{
				ADVECT_X_PRE(0);
				ADVECT_X_PRE(1);
				ADVECT_X_PRE(2);
				ADVECT_X_PRE(3);
				ADVECT_X_POST(0);
				ADVECT_X_POST(1);
				ADVECT_X_POST(2);
				ADVECT_X_POST(3);
				ADVECT_X_ASSIGN_SIMPLE(0);
				ADVECT_X_ASSIGN_SIMPLE(1);
				ADVECT_X_ASSIGN_SIMPLE(2);
				ADVECT_X_ASSIGN_SIMPLE(3);
				x+=4;
			}
			while(x<w-1)
			{
				ADVECT_X_PRE(0);
				ADVECT_X_POST(0);
				ADVECT_X_ASSIGN_SIMPLE(0);
				x++;
			}
			{
				ADVECT_X_PRE_N(0);
				ADVECT_X_POST(0);
				ADVECT_X_ASSIGN_SIMPLE(0);
			}
			
			
			x=0;
			{
				ADVECT_Y_PRE_0(0);
				ADVECT_Y_POST(0);
				ADVECT_Y_ASSIGN_SIMPLE(0);
			}
			x=1;
			while (x < w-4)
			{
				ADVECT_Y_PRE(0);
				ADVECT_Y_PRE(1);
				ADVECT_Y_PRE(2);
				ADVECT_Y_PRE(3);
				ADVECT_Y_POST(0);
				ADVECT_Y_POST(1);
				ADVECT_Y_POST(2);
				ADVECT_Y_POST(3);
				ADVECT_Y_ASSIGN_SIMPLE(0);
				ADVECT_Y_ASSIGN_SIMPLE(1);
				ADVECT_Y_ASSIGN_SIMPLE(2);
				ADVECT_Y_ASSIGN_SIMPLE(3);
				x+=4;
			}
			while (x < w-1)
			{
				ADVECT_Y_PRE(0);
				ADVECT_Y_POST(0);
				ADVECT_Y_ASSIGN_SIMPLE(0);
				x++;
			}
			{
				ADVECT_Y_PRE_N(0);
				ADVECT_Y_POST(0);
				ADVECT_Y_ASSIGN_SIMPLE(0);
			}
		}
#else
		if (d->clamp)
		{
			float w2 = (float)w-2;
			float h2 = (float)h-2;
			int x;
			float fx;
			for (x=1,fx=1; x<w-1; x++,fx++)
			{
				float vx = fluidFloatPointer(velX, x*sX + y*sY)[0];
				float vy = fluidFloatPointer(velY, x*sX + y*sY)[0];
				
				float dLeft = fluidFloatPointer(destX, (x-1)*sX + y*sY)[0];
				float dRight = fluidFloatPointer(destX, (x+1)*sX + y*sY)[0];
				
				float dUp = fluidFloatPointer(destX, x*sX + (y-1)*sY)[0];
				float dDown = fluidFloatPointer(destX, x*sX + (y+1)*sY)[0];
				
				// Note (x+1 - (x-1))/2 = 2/2 = 1
				// Note (x-x)/2 = 0
				fluidFloatPointer(destX, x*sX + y*sY)[0] +=
						timestep * vx * (dRight - dLeft)
						+ timestep *vy * (dDown - dUp);
				
				fluidFloatPointer(destX, x*sX + y*sY)[0] = 
					fminf(fmaxf(fluidFloatPointer(destX, x*sX + y*sY)[0],0),w2);
				
				
				fluidFloatPointer(destX, x*sX + y*sY)[0] = 
					fminf(fmaxf(fluidFloatPointer(destX, x*sX + y*sY)[0],fx-9),fx+9);
			}
			
			float fy = (float)y;
			for (x=1; x<w-1; x++)
			{
				float vx = fluidFloatPointer(velX, x*sX + y*sY)[0];
				float vy = fluidFloatPointer(velY, x*sX + y*sY)[0];
				
				float dLeft = fluidFloatPointer(destY, (x-1)*sX + y*sY)[0];
				float dRight = fluidFloatPointer(destY, (x+1)*sX + y*sY)[0];
				
				float dUp = fluidFloatPointer(destY, x*sX + (y-1)*sY)[0];
				float dDown = fluidFloatPointer(destY, x*sX + (y+1)*sY)[0];
				
				// Note (x+1 - (x-1))/2 = 2/2 = 1
				// Note (x-x)/2 = 0
				fluidFloatPointer(destY, x*sX + y*sY)[0] +=
						timestep * vx * (dRight - dLeft)
						+ timestep *vy * (dDown - dUp);
				
				fluidFloatPointer(destY, x*sX + y*sY)[0] = 
					fminf(fmaxf(fluidFloatPointer(destY, x*sX + y*sY)[0],0),h2);
				
				fluidFloatPointer(destY, x*sX + y*sY)[0] = 
					fminf(fmaxf(fluidFloatPointer(destY, x*sX + y*sY)[0],fy-9),fy+9);
			}
		}
		else
		{
			int x;
			fluidFloatPointer(destX, y*sY)[0] = 0;
			for (x=1; x<w-1; x++)
			{
				float vx = fluidFloatPointer(velX, x*sX + y*sY)[0];
				float vy = fluidFloatPointer(velY, x*sX + y*sY)[0];
				
				float dLeft = fluidFloatPointer(destX, (x-1)*sX + y*sY)[0];
				float dRight = fluidFloatPointer(destX, (x+1)*sX + y*sY)[0];
				
				float dUp = fluidFloatPointer(destX, x*sX + (y-1)*sY)[0];
				float dDown = fluidFloatPointer(destX, x*sX + (y+1)*sY)[0];
				
				// Note (x+1 - (x-1))/2 = 2/2 = 1
				// Note (x-x)/2 = 0
				fluidFloatPointer(destX, x*sX + y*sY)[0] +=
						timestep * vx * (dRight - dLeft)
						+ timestep *vy * (dDown - dUp);
			}
			fluidFloatPointer(destX, x*sX + y*sY)[0] = (float)(w-1);
			
			for (x=1; x<w-1; x++)
			{
				float vx = fluidFloatPointer(velX, x*sX + y*sY)[0];
				float vy = fluidFloatPointer(velY, x*sX + y*sY)[0];
				
				float dLeft = fluidFloatPointer(destY, (x-1)*sX + y*sY)[0];
				float dRight = fluidFloatPointer(destY, (x+1)*sX + y*sY)[0];
				
				float dUp = fluidFloatPointer(destY, x*sX + (y-1)*sY)[0];
				float dDown = fluidFloatPointer(destY, x*sX + (y+1)*sY)[0];
				
				// Note (x+1 - (x-1))/2 = 2/2 = 1
				// Note (x-x)/2 = 0
				fluidFloatPointer(destY, x*sX + y*sY)[0] +=
					timestep * vx * (dRight - dLeft)
					+ timestep *vy * (dDown - dUp);
			}
		}
#endif

	}
}

