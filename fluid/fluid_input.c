/*
 *  fluid_input.c
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
#include <stdlib.h>

void fluid_input_char2dens(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct video *v = &mode->video;
	
	int w = fieldWidth(v->f);
	
	float *f = fluidFloatPointer(fieldData(v->f),y*fieldStrideY(v->f));
	int c = fieldComponents(v->f);
	unsigned char *o = fieldCharData(v->o) + y*fieldStrideY(v->o);
	
	int x;
	for (x=0; x<w*c; x++)
	{
		float k = o[x];
		k/=255.0f;
		
		f[x] = f[x]*v->scale + (1-v->scale)*k;
	}
}


void fluid_input_float2vel(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct velocityIO *v = &mode->velocityIO;
	
	int w = fieldWidth(v->velX);
	
	float *fVelX = fluidFloatPointer(fieldData(v->velX),y*fieldStrideY(v->velX));
	float *fVelY = fluidFloatPointer(fieldData(v->velY),y*fieldStrideY(v->velY));
	float *fVelIn = fluidFloatPointer(fieldData(v->velIn),y*fieldStrideY(v->velIn));
	
	union {
		float f;
		int i;
	} t;
	
	int x;
	for (x=0; x<w; x++)
	{
		t.i = ntohl(((int*)fVelIn)[2*x]);
		fVelX[x] += t.f * v->scale;
	}
	
	for (x=0; x<w; x++)
	{
		t.i = ntohl(((int*)fVelIn)[2*x+1]);
		fVelY[x] += t.f * v->scale;
	}
}
