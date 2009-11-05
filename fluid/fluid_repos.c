/*
 *  fluid_repos.c
 *  FluidApp
 */

#include "fluid_macros_2.h"
#include "fluid_cpu.h"


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
		
		float nBackX = (int)fReposX;
		float nBackY = (int)fReposY;
		
		float scaleX = fReposX - (float)nBackX;
		float scaleY = fReposY - (float)nBackY;
		
		int offBackX = nBackX * dX;
		int offBackY = nBackY * dY;
		int offX2 = (nBackX+1) * dX;
		int offY2 = (nBackY+1) * dY;
		
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
}

