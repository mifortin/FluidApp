/*
 *  fluid_repos.c
 *  FluidApp
 */

#include "x_simd.h"


#include "fluid_macros_2.h"
#include "fluid_cpu.h"
#include <math.h>
#include <string.h>

//Simple repositioning of the data
void fluid_repos(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct repos *data = &mode->repos;
	
	int x;
	int w = fieldWidth(data->reposX);
	int w2 = w/4;
	int sY = fieldStrideY(data->reposX);
	
	int dX = fieldStrideX(data->src);
	int dY = fieldStrideY(data->src);
	int dC = fieldComponents(data->src);
	
	x128i vdX = {dX, dX, dX, dX};
	x128i vdY = {dY, dY, dY, dY};
	
	x128f *reposX = (x128f*)fluidFloatPointer(fieldData(data->reposX), y*sY);
	x128f *reposY = (x128f*)fluidFloatPointer(fieldData(data->reposY), y*sY);
	
	float *src = fieldData(data->src);
	float *dst = fieldData(data->dst);

	u128i offBackX, offBackY, offX2, offY2, d1, d2, d3, d4;
	u128f scaleX, scaleY;
	
	for (x=0; x<w2; x++)
	{
		x128f fReposX = reposX[x];
		x128f fReposY = reposY[x];
		
		x128i inBackX = x_ftoi(fReposX);
		x128i inBackY = x_ftoi(fReposY);
		
		x128f nBackX = x_itof(inBackX);
		x128f nBackY = x_itof(inBackY);
		
		scaleX.v = x_sub(fReposX, nBackX);
		scaleY.v = x_sub(fReposY, nBackY);
		
		offBackX.v = x_imul(inBackX, vdX);
		offBackY.v = x_imul(inBackY, vdY);
		offX2.v = x_iadd(offBackX.v, vdX);
		offY2.v = x_iadd(offBackY.v, vdY);
		
		d1.v = x_iadd(offBackX.v, offBackY.v);
		d2.v = x_iadd(offX2.v, offBackY.v);
		d3.v = x_iadd(offBackX.v, offY2.v);
		d4.v = x_iadd(offX2.v, offY2.v);
		
		int i;
		for (i=0; i<4; i++)
		{
			x128f *fDst = (x128f*)fluidFloatPointer(dst, (4*x+i)*dX + y*dY);
			
			x128f *bZZ = (x128f*)fluidFloatPointer(src, d1.i[i]);
			x128f *bOZ = (x128f*)fluidFloatPointer(src, d2.i[i]);
			x128f *bZO = (x128f*)fluidFloatPointer(src, d3.i[i]);
			x128f *bOO = (x128f*)fluidFloatPointer(src, d4.i[i]);
			
			x128f vSx = {scaleX.f[i], scaleX.f[i], scaleX.f[i], scaleX.f[i]};
			x128f vSy = {scaleY.f[i], scaleY.f[i], scaleY.f[i], scaleY.f[i]};
			
			int c;
			for (c=0; c<dC/4; c++)
			{
				//For each component, do standard advection!
				fDst[c] = x_madd(
							x_sub(x_simd_one, vSy),
							x_madd(
								x_sub(x_simd_one, vSx),
								bZZ[c],
								x_mul(vSx, bOZ[c])),
							x_mul(
								vSy,
								x_madd(
									x_sub(x_simd_one, vSx),
									bZO[c],
									x_mul(vSx,bOO[c]))));
			}
		}
	}
}

void fluid_reposVel(fluid *in_f, int y, pvt_fluidMode *mode)
{	struct repos *data = &mode->repos;
	
	int x, xIn;
	int origX, counter;
	int w = fieldWidth(data->reposX);
	int sX = fieldStrideX(data->reposX);
	int sY = fieldStrideY(data->reposX);
	
	x128i vsX = {sX, sX, sX, sX};
	x128i vsY = {sY, sY, sY, sY};
	
	x128f *reposX = (x128f*)fluidFloatPointer(fieldData(data->reposX), y*sY);
	x128f *reposY = (x128f*)fluidFloatPointer(fieldData(data->reposY), y*sY);
	float *srcX = fieldData(data->src);
	x128f *dstX = (x128f*)fluidFloatPointer(fieldData(data->dst), y*sY);
	float *srcY = fieldData(data->src2);
	x128f *dstY = (x128f*)fluidFloatPointer(fieldData(data->dst2), y*sY);
	
	int w2 = w/4;
	
	u128i offBackX, offBackY, offX2, offY2, d1, d2, d3, d4;
	u128f scaleX, scaleY,dest[16];
	
	for (x=0; x<w2;)
	{
		xIn = x + 8;
		origX = x;
		counter = 0;
		for (; x<xIn; x++)
		{
			x128f fReposX = reposX[x];
			x128f fReposY = reposY[x];
			
			x128i inBackX = x_ftoi(fReposX);
			x128i inBackY = x_ftoi(fReposY);
			
			x128f nBackX = x_itof(inBackX);
			x128f nBackY = x_itof(inBackY);
			
			scaleX.v = x_sub(fReposX, nBackX);
			scaleY.v = x_sub(fReposY, nBackY);
			
			offBackX.v = x_imul(inBackX, vsX);
			offBackY.v = x_imul(inBackY, vsY);
			offX2.v = x_iadd(offBackX.v, vsX);
			offY2.v = x_iadd(offBackY.v, vsY);
			
			d1.v = x_iadd(offBackX.v, offBackY.v);
			d2.v = x_iadd(offX2.v, offBackY.v);
			d3.v = x_iadd(offBackX.v, offY2.v);
			d4.v = x_iadd(offX2.v, offY2.v);
			
			int i;
			for (i=0; i<4; i++)
			{
				float *bZZ = fluidFloatPointer(srcX, d1.i[i]);
				float *bOZ = fluidFloatPointer(srcX, d2.i[i]);
				float *bZO = fluidFloatPointer(srcX, d3.i[i]);
				float *bOO = fluidFloatPointer(srcX, d4.i[i]);
				
				//For each component, do standard advection!
				dest[counter].f[i] = fluidLinearInterpolation(scaleX.f[i], scaleY.f[i],
												bZZ[0], bOZ[0], bZO[0], bOO[0]);
				
				
				bZZ = fluidFloatPointer(srcY, d1.i[i]);
				bOZ = fluidFloatPointer(srcY, d2.i[i]);
				bZO = fluidFloatPointer(srcY, d3.i[i]);
				bOO = fluidFloatPointer(srcY, d4.i[i]);
				
				//For each component, do standard advection!
				dest[counter+1].f[i] = fluidLinearInterpolation(scaleX.f[i], scaleY.f[i],
												bZZ[0], bOZ[0], bZO[0], bOO[0]);
			}
			
			counter += 2;
		}
		
		x=origX;
		counter=0;
		for(;x<xIn;x++)
		{
			dstX[x] = dest[counter].v;
			counter+=2;
		}
		
		x=origX;
		counter=1;
		for(;x<xIn;x++)
		{
			dstY[x] = dest[counter].v;
			counter+=2;
		}
	}
}

