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

#include "fluid_macros_2.h"
#include "fluid_cpu.h"

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
	vector unsigned char *vo = (vector unsigned char*)o;
	
	vector short min = {0,0,0,0,0,0,0,0};
	vector short max = {255,255,255,255,255,255,255,255};
	
	for (x=0; x<w*c; x++)
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
	
	for (x=0; x<w*c; x++)
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