/*
 *  fluid_repos.c
 *  FluidApp
 */

#include "fluid_macros_2.h"
#include "fluid_cpu.h"
#include <math.h>


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
	
	for (x=0; x<w; x++)
	{
		float fReposX = fluidFloatPointer(reposX, x*sX + y*sY)[0];
		float fReposY = fluidFloatPointer(reposY, x*sX + y*sY)[0];
		
		float *fDst = fluidFloatPointer(dst, x*dX + y*dY);
		
		float nBackX = floorf(fReposX);
		float nBackY = floorf(fReposY);
		
		float scaleX = fReposX - nBackX;
		float scaleY = fReposY - nBackY;
		
		int inBackX = (int)fReposX;
		int inBackY = (int)fReposY;
		
		int offBackX = inBackX * dX;
		int offBackY = inBackY * dY;
		int offX2 = (inBackX+1) * dX;
		int offY2 = (inBackY+1) * dY;
		
#ifdef __APPLE_ALTIVEC__
		vector float *bZZ = (vector float*)fluidFloatPointer(src, offBackX + offBackY);
		vector float *bOZ = (vector float*)fluidFloatPointer(src, offX2 + offBackY);
		vector float *bZO = (vector float*)fluidFloatPointer(src, offBackX + offY2);
		vector float *bOO = (vector float*)fluidFloatPointer(src, offX2 + offY2);
		
		vector float vSx = {scaleX, scaleX, scaleX, scaleX};
		vector float vSy = {scaleY, scaleY, scaleY, scaleY};
		vector float vOne = {1,1,1,1};
		vector float vZero = {0,0,0,0};
		
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
#else
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
#endif
	}
}

