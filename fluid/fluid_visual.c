/*
 *  fluid_visual.c
 *  FluidApp
 */

#ifdef __SSE3__
#include <xmmintrin.h>
#include <emmintrin.h>
#include <pmmintrin.h>
//#undef __SSE3__
#endif

#ifdef CELL
#include "altivec.h"
#undef __APPLE_ALTIVEC__
#endif

#include "fluid_macros_2.h"
#include "fluid_cpu.h"
#include <stdlib.h>

void fluid_video_dens2char(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct video *v = &mode->video;
	
	int w = fieldWidth(v->f);
	
	float *f = fluidFloatPointer(fieldData(v->f),y*fieldStrideY(v->f));
	int c = fieldComponents(v->f);
	unsigned char *o = fieldCharData(v->o) + y*fieldStrideY(v->o);
	
#ifdef __APPLE_ALTIVEC__
	int x;
	w = w * c / (4*4);
	
	vector float *vf = (vector float*)f;
	vector signed char *vo = (vector signed char*)o;
	
	vector short min = {0,0,0,0,0,0,0,0};
	vector short max = {255,255,255,255,255,255,255,255};
	
	for (x=0; x<w; x++)
	{
		vector int i1 = vec_cts(vf[x*4+0], 8);
		vector int i2 = vec_cts(vf[x*4+1], 8);
		vector int i3 = vec_cts(vf[x*4+2], 8);
		vector int i4 = vec_cts(vf[x*4+3], 8);
		
		vector short s1 = vec_pack(i1, i2);
		vector short s2 = vec_pack(i3, i4);
		s1 = vec_min(vec_max(s1, min), max);
		s2 = vec_min(vec_max(s2, min), max);
		
		vo[x] = vec_pack(s1,s2);
	}
#elif __SSE3__
	int x;
	w = w * c / (4*4);
	
	__m128 *vf = (__m128*)f;
	__m128i *vo = (__m128i*)o;
	
	__m128 max = {255,255,255,255};
	
	for (x=0; x<w; x++)
	{
		__m128i i1 = _mm_cvtps_epi32(_mm_mul_ps(vf[x*4+0],max));
		__m128i i2 = _mm_cvtps_epi32(_mm_mul_ps(vf[x*4+1],max));
		__m128i i3 = _mm_cvtps_epi32(_mm_mul_ps(vf[x*4+2],max));
		__m128i i4 = _mm_cvtps_epi32(_mm_mul_ps(vf[x*4+3],max));
		
		i1 = _mm_packs_epi32(i1,i2);
		i3 = _mm_packs_epi32(i3,i4);
		
		vo[x] = _mm_packus_epi16(i1, i3);
	}
#else
	int x;
	for (x=0; x<w*c; x++)
	{
		unsigned int i = f[x]*256;
		if (i<0) i =0;
		if (i>255) i = 255;
		
		o[x] = (unsigned char)i;
	}
#endif
}

void fluid_input_vel2float(fluid *in_f, int y, pvt_fluidMode *mode)
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
		t.i = htonl(((int*)fVelX)[x]);
		fVelIn[2*x] = t.f;
		
		t.i = htonl(((int*)fVelY)[x]);
		fVelIn[2*x+1] = t.f;
	}
}


void fluid_input_vel2float_scale(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct velocityIO *v = &mode->velocityIO;
	
	int w = fieldWidth(v->velX);
	
	int w2 = fieldWidth(v->velIn);
	
	int h = fieldHeight(v->velX);
	int h2 = fieldHeight(v->velIn);
	
	int y2 = y*h/h2;
	
	if (y2 >= h)		return;
	
	float *fVelX = fluidFloatPointer(fieldData(v->velX),y2*fieldStrideY(v->velX));
	float *fVelY = fluidFloatPointer(fieldData(v->velY),y2*fieldStrideY(v->velY));
	float *fVelIn = fluidFloatPointer(fieldData(v->velIn),y*fieldStrideY(v->velIn));
	
	union {
		float f;
		int i;
	} t;
	
	int x;
	for (x=0; x<w2; x++)
	{
		t.i = htonl(((int*)fVelX)[x*w/w2]);
		fVelIn[2*x] = t.f;
		
		t.i = htonl(((int*)fVelY)[x*w/w2]);
		fVelIn[2*x+1] = t.f;
	}
}
