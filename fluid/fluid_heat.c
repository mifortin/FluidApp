/*
 *  fluid_heat.c
 *  FluidApp
 *
 */

#include "x_simd.h"

#include "fluid_macros_2.h"
#include "fluid_cpu.h"
#include <stdlib.h>

void fluid_force_heat(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct temperature *t = &mode->temperature;
	
	int w = fieldWidth(t->density);
	
	float *f = fluidFloatPointer(fieldData(t->density),y*fieldStrideY(t->density));
	int c = fieldComponents(t->density);
	
	float *vx = fluidFloatPointer(fieldData(t->velX),y*fieldStrideY(t->velX));
	float *vy = fluidFloatPointer(fieldData(t->velY),y*fieldStrideY(t->velY));
	
	float alpha = t->alpha;
	float beta = t->beta;
	float ambient = t->ambient;
	
	float upX = t->upX;
	float upY = t->upY;
	
	int x;
	for (x=0; x<w; x++)
	{
		vx[x] += alpha * upX + beta*(f[x*c]-f[x*c+2]-ambient) * upX;
		vy[x] += alpha * upY + beta*(f[x*c]-f[x*c+2]-ambient) * upY;
	}
}
