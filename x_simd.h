/*
 *  x_simd.h
 *  FluidApp
 */

//Basic compatibility functions to write SIMD code across the various platforms
//	- also provide a means to help the compiler do SIMD code...

#ifndef X_SIMD_H
#define X_SIMD_H

//Get the needed libraries
#ifdef __APPLE_ALTIVEC__
	#ifdef CELL
		#include "altivec.h"
	#endif
	#define X_SIMD
#elif defined __SSE3__
	#include <xmmintrin.h>
	#include <emmintrin.h>
	#include <pmmintrin.h>
	#include <tmmintrin.h>
	#define X_SIMD
#endif

//Floating-point
#ifdef __APPLE_ALTIVEC__
	typedef vector float x128f;
#elif defined __SSE3__
	typedef __m128 x128f;
#else
	typedef struct { float a,b,c,d; } x128f;
#endif

//x_madd
#ifdef __APPLE_ALTIVEC__
	#define x_madd(a,b,c) vec_madd(a,b,c)
#elif defined __SSE3__
	#define x_madd(a,b,c) _mm_add_ps(c, _mm_mul_ps(a,b))
#else
	inline x128f x_madd(x128f a, x128f b, x128f c)
	{
		return (x128f)
				{a.a*b.a+c.a,
				a.b*b.b+c.b,
				a.c*b.c+c.c,
				a.d*b.d+c.d};
	}
#endif

//x_add
#ifdef __APPLE_ALTIVEC__
	#define x_add(a,b) vec_add(a,b)
#elif defined __SSE3__
	#define x_add(a,b) _mm_add_ps(a,b)
#else
	inline x128f x_add(x128f a, x128f b)
	{
		return (x128f)
				{a.a+b.a,
				a.b+b.b,
				a.c+b.c,
				a.d+b.d};
	}
#endif

//x_dun
#ifdef __APPLE_ALTIVEC__
	#define x_sub(a,b) vec_sub(a,b)
#elif defined __SSE3__
	#define x_sub(a,b) _mm_sub_ps(a,b)
#else
	inline x128f x_sub(x128f a, x128f b)
	{
		return (x128f)
			{a.a-b.a,
				a.b-b.b,
				a.c-b.c,
				a.d-b.d};
	}
#endif

//x_mul
#ifdef __APPLE_ALTIVEC__
	static const x128f x_simd_zero = {0,0,0,0};
	#define x_mul(a,b) vec_madd(a,b,x_simd_zero)
#elif defined __SSE3__
	#define x_mul(a,b) _mm_mul_ps(a,b)
#else
	inline x128f x_mul(x128f a, x128f b)
	{
		return (x128f)
				{a.a*b.a,
					a.b*b.b,
					a.c*b.c,
					a.d*b.d};
	}
#endif

//x_min
#ifdef __APPLE_ALTIVEC__
	#define x_min(a,b) vec_min(a,b)
#elif defined __SSE3__
	#define x_min(a,b) _mm_min_ps(a,b)
#else
	inline x128f x_min(x128f a, x128f b)
	{
		return (x128f)
			{	a.a<b.a?a.a:b.a,
				a.b<b.b?a.b:b.b,
				a.c<b.c?a.c:b.c,
				a.d<b.d?a.d:b.d		};
	}
#endif

//x_max
#ifdef __APPLE_ALTIVEC__
	#define x_max(a,b) vec_max(a,b)
#elif defined __SSE3__
	#define x_max(a,b) _mm_max_ps(a,b)
#else
	inline x128f x_max(x128f a, x128f b)
	{
		return (x128f)
			{	a.a>b.a?a.a:b.a,
				a.b>b.b?a.b:b.b,
				a.c>b.c?a.c:b.c,
				a.d>b.d?a.d:b.d		};
	}
#endif

//x_sld
#ifdef __APPLE_ALTIVEC__
	#define x_sld(a,b,amt)	vec_sld(a,b,amt)
#elif defined __SSE3__
	#define x_sld(a,b,amt)	x_add(_mm_srli_sf128(a,amt), _mm_slli_sf128(b,16-amt))
#else
	inline x128f x_sld(x128f a, x128f b, int c)
	{
		while (c>0)
		{
			a.a = a.b;
			a.b = a.c;
			a.c = a.d;
			a.d = b.a;
			b.a = b.b;
			b.b = b.c;
			b.c = b.d;
			
			c-=4;
		}
		
		return a;
	}
#endif

#endif
