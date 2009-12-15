/*
 *  fluid_advection_fwd.c
 *  FluidApp
 *
 */

#include "fluid_macros_2.h"
#include "fluid_cpu.h"
#include <math.h>

void fluid_advection_fwd_velocity(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct advection_stam_velocity *d = &mode->advection_stam_velocity;
	
	int w = fieldWidth(d->srcVelX);
	int h = fieldHeight(d->srcVelY);
	
	float *velX		= fieldData(d->srcVelX);
	float *velY		= fieldData(d->srcVelY);
	
	float *velDestX	= fieldData(d->dstVelX);
	float *velDestY	= fieldData(d->dstVelY);
	
	
	int sY = fieldStrideY(d->srcVelY);
	
	float timestep = -d->timestep / 18.0f;	// (2.0f * 9.0f)
	
	int x;
	if (y==0)
	{
	}
	else if (y == h-1)
	{
	}
	else
	{
#ifdef __APPLE_ALTIVEC__
		
		vector float *vVelX = (vector float*)fluidFloatPointer(velX, y*sY);
		vector float *vVelY = (vector float*)fluidFloatPointer(velY, y*sY);
		
		vector float *vVelYP = (vector float*)fluidFloatPointer(velY, (y-1)*sY);
		vector float *vVelYN = (vector float*)fluidFloatPointer(velY, (y+1)*sY);
		
		vector float *vVelXP = (vector float*)fluidFloatPointer(velX, (y-1)*sY);
		vector float *vVelXN = (vector float*)fluidFloatPointer(velX, (y+1)*sY);
		
		vector float *vDVelX = (vector float*)fluidFloatPointer(velDestX, y*sY);
		vector float *vDVelY = (vector float*)fluidFloatPointer(velDestY, y*sY);
		
		vector float vT = {timestep, timestep, timestep, timestep};
		vector float vZero = {0,0,0,0};
		vector float vPN = {9,9,9,9};
		vector float vNN = {-9,-9,-9,-9};
		
#define ADVECT_X_PRE(n)															\
		vector float sl_vx ## n = vec_sld(vVelX[x + n], vVelX[x+1 + n],4);		\
		vector float sr_vx ## n = vec_sld(vVelX[x-1 + n], vVelX[x + n], 12);
		
#define ADVECT_Y_PRE(n)															\
		vector float sl_vy ## n = vec_sld(vVelY[x + n], vVelY[x+1 + n],4);		\
		vector float sr_vy ## n = vec_sld(vVelY[x-1 + n], vVelY[x + n], 12);
		
#define ADVECT_X_POST(n)												\
		vDVelX[x+n] = vec_madd(vT, vec_madd(vVelX[x+n],					\
							vec_sub(sl_vx ## n, sr_vx ## n),			\
								vec_madd(vVelY[x+n],					\
									vec_sub(vVelXN[x+n], vVelXP[x+n]),	\
										 vZero)),						\
							vVelX[x+n]);								\
		vDVelX[x+n] = vec_min(vec_max(vDVelX[x+n], vNN), vPN);
		
#define ADVECT_Y_POST(n)											\
		vDVelY[x+n] = vec_madd(vT, vec_madd(vVelX[x+n],				\
						vec_sub(sl_vy ## n, sr_vy ## n),			\
							vec_madd(vVelY[x+n],					\
								vec_sub(vVelYN[x+n], vVelYP[x+n]),	\
									vZero)),						\
						vVelY[x+n]);								\
		vDVelY[x+n] = vec_min(vec_max(vDVelY[x+n], vNN), vPN);
							
		x = 1;
		/*while (x<w/4-1 && x%4 != 0)
		{
			ADVECT_X_PRE(0)
			ADVECT_X_POST(0)
			
			x++;
		}
		while (x<w/4-5)
		{
			ADVECT_X_PRE(0)
			ADVECT_X_PRE(1)
			ADVECT_X_PRE(2)
			ADVECT_X_PRE(3)
			
			ADVECT_X_POST(0)
			ADVECT_X_POST(1)
			ADVECT_X_POST(2)
			ADVECT_X_POST(3)
			
			x+=4;
		}*/
		while (x<w/4-1)
		{
			ADVECT_X_PRE(0)
			ADVECT_X_POST(0)
			
			x++;
		}
		
		x = 1;
		/*while (x < w/2 - 1 && x%4 != 0)
		{			
			ADVECT_Y_PRE(0)
			ADVECT_Y_POST(0)
			x++;
		}
		while (x<w/2-5)
		{
			ADVECT_Y_PRE(0)
			ADVECT_Y_PRE(1)
			ADVECT_Y_PRE(2)
			ADVECT_Y_PRE(3)
			
			ADVECT_Y_POST(0)
			ADVECT_Y_POST(1)
			ADVECT_Y_POST(2)
			ADVECT_Y_POST(3)
			
			x+=4;
		}*/
		while (x < w/4 - 1)
		{			
			ADVECT_Y_PRE(0)
			ADVECT_Y_POST(0)
			x++;
		}
#else
		int sX = fieldStrideX(d->srcVelX);
		for (x=1; x<w-1; x++)
		{
			float *fVelDestX = fluidFloatPointer(velDestX, x*sX + y*sY);
			float fDataXM = fluidFloatPointer(velX, (x-1)*sX + y*sY)[0];
			float fDataX = fluidFloatPointer(velX, x*sX + y*sY)[0];
			float fDataXP = fluidFloatPointer(velX, (x+1)*sX + y*sY)[0];
			
			float fDataXM2 = fluidFloatPointer(velX, (x)*sX + (y-1)*sY)[0];
			float fDataXP2 = fluidFloatPointer(velX, (x)*sX + (y+1)*sY)[0];
			
			float fDataY = fluidFloatPointer(velY, x*sX + y*sY)[0];
			
			fVelDestX[0] = fDataX * timestep * (fDataXP - fDataXM)
							+ fDataY * timestep * (fDataXP2 - fDataXM2)
							+ fDataX;
			
			if (fVelDestX[0] < -9)	fVelDestX[0] = -9;
			if (fVelDestX[0] > 9)	fVelDestX[0] = 9;
		}
		
		for (x=1; x<w-1; x++)
		{
			float *fVelDestY = fluidFloatPointer(velDestY, x*sX + y*sY);
			float fDataYM = fluidFloatPointer(velY, (x)*sX + (y-1)*sY)[0];
			float fDataY = fluidFloatPointer(velY, x*sX + y*sY)[0];
			float fDataYP = fluidFloatPointer(velY, (x)*sX + (y+1)*sY)[0];
			
			
			float fDataYMX = fluidFloatPointer(velY, (x-1)*sX + y*sY)[0];
			float fDataYPX = fluidFloatPointer(velY, (x+1)*sX + y*sY)[0];
			
			float fDataX = fluidFloatPointer(velX, x*sX + y*sY)[0];
			
			fVelDestY[0] = fDataY * timestep * (fDataYP - fDataYM)
							+ fDataX * timestep * (fDataYPX - fDataYMX)
							+ fDataY;
			
			if (fVelDestY[0] < -9)	fVelDestY[0] = -9;
			if (fVelDestY[0] > 9)	fVelDestY[0] = 9;
		}
#endif
	}
}


void fluid_advection_fwd_dens(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct repos *data = &mode->repos;
	
	int w = fieldWidth(data->reposX);
	int h = fieldHeight(data->reposX);
	int sX = fieldStrideX(data->reposX);
	int sY = fieldStrideY(data->reposX);
	
	int dX = fieldStrideX(data->src);
	int dY = fieldStrideY(data->src);
	int dC = fieldComponents(data->src);
	
	float *reposX = fieldData(data->reposX);
	float *reposY = fieldData(data->reposY);
	float *src = fieldData(data->src);
	float *dst = fieldData(data->dst);
	
	float timestep = -data->timestep / 18.0f;
	
	int x;
	if (y==0)
	{
	}
	else if (y == h-1)
	{
	}
	else
	{
		for (x=1; x<w-1; x++)
		{
			int c;
			
			float velX = fluidFloatPointer(reposX, x*sX + y*sY)[0];
			float velY = fluidFloatPointer(reposY, x*sX + y*sY)[0];
			
			for (c=0; c<dC; c++)
			{
				//Compute change of C...
				float a00 = fluidFloatPointer(src, dX*x + dY*y)[c];
				float aM0 = fluidFloatPointer(src, dX*(x-1) + dY*y)[c];
				float aP0 = fluidFloatPointer(src, dX*(x+1) + dY*y)[c];
				float a0M = fluidFloatPointer(src, dX*x + dY*(y-1))[c];
				float a0P = fluidFloatPointer(src, dX*x + dY*(y+1))[c];
				
				float *dest = fluidFloatPointer(dst, dX*x + dY*y);
				dest[c] =
					a00 + timestep * (velX * (aP0 - aM0) + velY * (a0P - a0M));
				
				if (dest[c] < 0)	dest[c] = 0;
				else if (dest[c] > 1)	dest[c] = 1;
			}
		}
		
		if (data->clamp == 1)
		{
			for (x=1; x<w-1; x++)
			{
				int c;
				
				for (c=0; c<dC; c++)
				{					
					float *dest = fluidFloatPointer(dst, dX*x + dY*y);
					
					if (dest[c] < 0)	dest[c] = 0;
					else if (dest[c] > 1)	dest[c] = 1;
				}
			}
		}
	}
}
