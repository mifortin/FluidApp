/*
 *  fluid_visual.c
 *  FluidApp
 */

#include "x_simd.h"

#include "fluid_macros_2.h"
#include "fluid_cpu.h"
#include <stdlib.h>

void fluid_video_temp2char(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct video *v = &mode->video;
	
	const int w = fieldWidth(v->f);
	
	const float *f = fluidFloatPointer(fieldData(v->f), y*fieldStrideY(v->f));
	const int c = fieldComponents(v->f);
	unsigned char *o = fieldCharData(v->o) + y*fieldStrideY(v->o);
	
	int x;
	for (x=0; x<w; x++)
	{
		unsigned int ir = ((f[x*c+0]-f[x*c+2])*128.0f + 128.0f);
		
		if (ir < 0)	ir = 0;
		if (ir > 255)	ir = 255;
		
		o[x*c+0] = (unsigned char)(in_f->m_tempGrad[ir].f[0]*255);
		o[x*c+1] = (unsigned char)(in_f->m_tempGrad[ir].f[1]*255);
		o[x*c+2] = (unsigned char)(in_f->m_tempGrad[ir].f[2]*255);
		o[x*c+3] = (unsigned char)(in_f->m_tempGrad[ir].f[3]*255);
	}
}

void fluid_video_dens2char(fluid *in_f, int y, pvt_fluidMode *mode)
{
	struct video *v = &mode->video;
	
	const int w = fieldWidth(v->f);
	
	const float *f = fluidFloatPointer(fieldData(v->f),y*fieldStrideY(v->f));
	const int c = fieldComponents(v->f);
	unsigned char *o = fieldCharData(v->o) + y*fieldStrideY(v->o);
	
#ifdef __APPLE_ALTIVEC__
	int x;
	const int w2 = w * c / (4*4);
	
	const vector float *vf = (vector float*)f;
	vector signed char *vo = (vector signed char*)o;
	
	const vector short min = {0,0,0,0,0,0,0,0};
	const vector short max = {255,255,255,255,255,255,255,255};
	
	for (x=0; x<w2; x++)
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
	const int w2 = w * c / (4*4);
	
	const __m128 *vf = (__m128*)f;
	__m128i *vo = (__m128i*)o;
	
	const __m128 max = {255,255,255,255};
	
	for (x=0; x<w2; x++)
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
