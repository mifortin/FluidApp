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
	int sX = fieldStrideX(data->reposX);
	int sY = fieldStrideY(data->reposX);
	
	int dX = fieldStrideX(data->src);
	int dY = fieldStrideY(data->src);
	int dC = fieldComponents(data->src);
	
	float *reposX = fieldData(data->reposX);
	float *reposY = fieldData(data->reposY);
	float *src = fieldData(data->src);
	float *dst = fieldData(data->dst);
	
#ifdef __APPLE_ALTIVEC__
	vector float vOne = {1,1,1,1};
	vector float vZero = {0,0,0,0};
#elif defined __SSE3__
	__m128 vOne = {1,1,1,1};
#endif
	
#ifdef __APPLE_ALTIVEC__
	for (x=0; x<w; x++)
	{
		float fReposX = fluidFloatPointer(reposX, x*sX + y*sY)[0];
		float fReposY = fluidFloatPointer(reposY, x*sX + y*sY)[0];
		
		float *fDst = fluidFloatPointer(dst, x*dX + y*dY);
		
		int inBackX = (int)fReposX;
		int inBackY = (int)fReposY;
		
		float nBackX = (float)(inBackX);
		float nBackY = (float)(inBackY);
		
		float scaleX = fReposX - nBackX;
		float scaleY = fReposY - nBackY;
		
		int offBackX = inBackX * dX;
		int offBackY = inBackY * dY;
		int offX2 = offBackX + dX;
		int offY2 = offBackY + dY;
		
		vector float *bZZ = (vector float*)fluidFloatPointer(src, offBackX + offBackY);
		vector float *bOZ = (vector float*)fluidFloatPointer(src, offX2 + offBackY);
		vector float *bZO = (vector float*)fluidFloatPointer(src, offBackX + offY2);
		vector float *bOO = (vector float*)fluidFloatPointer(src, offX2 + offY2);
		
		vector float vSx = {scaleX, scaleX, scaleX, scaleX};
		vector float vSy = {scaleY, scaleY, scaleY, scaleY};
		
		vector float *vDst = (vector float*)fDst;
		
		int c;
		for (c=0; c<dC/4; c++)
		{
			vDst[c] = vec_madd(
							vec_sub(vOne, vSy),
							vec_madd(
								vec_sub(vOne, vSx),
								bZZ[c],
								vec_madd(vSx, bOZ[c], vZero)),
						vec_madd(
							vSy,
							vec_madd(
								vec_sub(vOne, vSx),
								bZO[c],
								vec_madd(vSx,bOO[c],vZero)),
							vZero));
		}
	}
#elif defined __SSE3__
	
#define SSE_REPOS_0(n)													\
	float fReposX ## n = fluidFloatPointer(reposX, x*sX + y*sY)[n];		\
	float fReposY ## n = fluidFloatPointer(reposY, x*sX + y*sY)[n];		\
																		\
	float *fDst ## n = fluidFloatPointer(dst, (x+n)*dX + y*dY);
	
#define SSE_REPOS_1(n)													\
	int inBackX ## n = (int)fReposX ## n;								\
	int inBackY ## n = (int)fReposY ## n;								\
																		\
	float nBackX ## n = (float)(inBackX ## n);							\
	float nBackY ## n = (float)(inBackY ## n);
	
#define SSE_REPOS_2(n)													\
	float scaleX ## n = fReposX ## n - nBackX ## n;						\
	float scaleY ## n = fReposY ## n - nBackY ## n;						\
																		\
	int offBackX ## n = inBackX ## n * dX;								\
	int offBackY ## n = inBackY ## n * dY;								\
	int offX2 ## n = offBackX ## n + dX;								\
	int offY2 ## n = offBackY ## n + dY;
	
#define SSE_REPOS_3(n)																		\
	__m128 *bZZ ## n = (__m128*)fluidFloatPointer(src, offBackX ## n + offBackY ## n);		\
	__m128 *bOZ ## n = (__m128*)fluidFloatPointer(src, offX2 ## n + offBackY ## n);			\
	__m128 *bZO ## n = (__m128*)fluidFloatPointer(src, offBackX ## n + offY2 ## n);			\
	__m128 *bOO ## n = (__m128*)fluidFloatPointer(src, offX2 ## n + offY2 ## n);			\
																							\
	__builtin_prefetch(bZZ ## n);															\
	__builtin_prefetch(bZO ## n);															\
																							\
	__m128 *vDst ## n = (__m128*)fDst ## n;													\
	__m128 vSx ## n = {scaleX ## n, scaleX ## n, scaleX ## n, scaleX ## n};					\
	__m128 vSy ## n = {scaleY ## n, scaleY ## n, scaleY ## n, scaleY ## n};					\
	

#define SSE_REPOS_4(n)															\
	vDst ## n[c] = _mm_add_ps(_mm_mul_ps(										\
						_mm_sub_ps(vOne, vSy ## n),								\
						_mm_add_ps(_mm_mul_ps(									\
											  _mm_sub_ps(vOne, vSx ## n),		\
											  bZZ ## n[c]),						\
								   _mm_mul_ps(vSx ## n, bOZ ## n[c]))),			\
			 _mm_mul_ps(														\
						vSy ## n,												\
						_mm_add_ps(_mm_mul_ps(									\
											  _mm_sub_ps(vOne, vSx ## n),		\
											  bZO ## n[c]),						\
								   _mm_mul_ps(vSx ## n,bOO ## n[c]))			\
						));
	
	x=0;
	while(x<w-2)
	{
		SSE_REPOS_0(0)
		SSE_REPOS_1(0)
		SSE_REPOS_2(0)
		SSE_REPOS_3(0)
		
		SSE_REPOS_0(1)
		SSE_REPOS_1(1)
		SSE_REPOS_2(1)
		SSE_REPOS_3(1)
		
		//SSE_REPOS_0(2)
		//SSE_REPOS_1(2)
		//SSE_REPOS_2(2)
		//SSE_REPOS_3(2)
		
		//SSE_REPOS_0(3)
		//SSE_REPOS_1(3)
		//SSE_REPOS_2(3)
		//SSE_REPOS_3(3)
		
		int c=0;
		for (c=0; c<dC/4; c++)
		{
			SSE_REPOS_4(0)
			SSE_REPOS_4(1)
			//SSE_REPOS_4(2)
			//SSE_REPOS_4(3)
		}
		x+=2;
	}
	while(x<w)
	{
		SSE_REPOS_0(0)
		SSE_REPOS_1(0)
		SSE_REPOS_2(0)
		SSE_REPOS_3(0)
		
		int c;
		for (c=0; c<dC/4; c++)
		{
			SSE_REPOS_4(0)
		}
		x++;
	}
#else
	for (x=0; x<w; x++)
	{
		float fReposX = fluidFloatPointer(reposX, x*sX + y*sY)[0];
		float fReposY = fluidFloatPointer(reposY, x*sX + y*sY)[0];
		
		float *fDst = fluidFloatPointer(dst, x*dX + y*dY);
		
		int inBackX = (int)fReposX;
		int inBackY = (int)fReposY;
		
		float nBackX = (float)(inBackX);
		float nBackY = (float)(inBackY);
		
		float scaleX = fReposX - nBackX;
		float scaleY = fReposY - nBackY;
		
		int offBackX = inBackX * dX;
		int offBackY = inBackY * dY;
		int offX2 = offBackX + dX;
		int offY2 = offBackY + dY;
		
		float *bZZ = fluidFloatPointer(src, offBackX + offBackY);
		float *bOZ = fluidFloatPointer(src, offX2 + offBackY);
		float *bZO = fluidFloatPointer(src, offBackX + offY2);
		float *bOO = fluidFloatPointer(src, offX2 + offY2);
		
		int c;
		for (c=0; c<dC; c++)
		{
			//For each component, do standard advection!
			fDst[c] = fluidLinearInterpolation(scaleX, scaleY,
											bZZ[c], bOZ[c], bZO[c], bOO[c]);
		}
	}
#endif
}

void fluid_reposVel(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct repos *data = &mode->repos;
	
	int x;
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
	
	u128i offBackX, offBackY, offX2, offY2;
	u128f scaleX, scaleY, dx, dy;
	
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
		
		offBackX.v = x_imul(inBackX, vsX);
		offBackY.v = x_imul(inBackY, vsY);
		offX2.v = x_iadd(offBackX.v, vsX);
		offY2.v = x_iadd(offBackY.v, vsY);
		
		int i;
		for (i=0; i<4; i++)
		{
			float *bZZ = fluidFloatPointer(srcX, offBackX.i[i] + offBackY.i[i]);
			float *bOZ = fluidFloatPointer(srcX, offX2.i[i] + offBackY.i[i]);
			float *bZO = fluidFloatPointer(srcX, offBackX.i[i] + offY2.i[i]);
			float *bOO = fluidFloatPointer(srcX, offX2.i[i] + offY2.i[i]);
			
			//For each component, do standard advection!
			dx.f[i] = fluidLinearInterpolation(scaleX.f[i], scaleY.f[i],
											bZZ[0], bOZ[0], bZO[0], bOO[0]);
			
			
			bZZ = fluidFloatPointer(srcY, offBackX.i[i] + offBackY.i[i]);
			bOZ = fluidFloatPointer(srcY, offX2.i[i] + offBackY.i[i]);
			bZO = fluidFloatPointer(srcY, offBackX.i[i] + offY2.i[i]);
			bOO = fluidFloatPointer(srcY, offX2.i[i] + offY2.i[i]);
			
			//For each component, do standard advection!
			dy.f[i] = fluidLinearInterpolation(scaleX.f[i], scaleY.f[i],
											bZZ[0], bOZ[0], bZO[0], bOO[0]);
		}
		
		dstX[x] = dx.v;
		dstY[x] = dy.v;
	}
}

