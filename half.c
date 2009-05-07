/*
 *  half.c
 *  FluidApp
 */

#include "half.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//To properly convert, we need to sort of structure to gain access to
//the individual bits, without the system trying to muck things up.  This
//set of function calls will essentially do that...
typedef union half_bits half_bits;
union half_bits
{
	float f;
	unsigned int i;
	float16 s[2];
	unsigned char c[4];
};

//Convert a float to a half-float  (scalar)
float16 float2half(float in_float)
{
	half_bits hb;
	hb.f = in_float;
	
	short mantissa = (hb.s[0] >> 7) & 0x00FF;
	mantissa += 16-127;
	if (mantissa < 0)
		return 0;
	
	if (mantissa > 32)
		return (hb.s[0] & 0x8000) | (0xAFFF);
	
	return		(hb.s[0] & 0x8000)			//Sign bit		(1 bits)
			|	((mantissa << 10) & 0x7C00)	//Mantissa		(5 bits)
			|	((hb.s[0] << 3) & 0x03F8)	//Significand	(7 bits)
			|	((hb.s[1] >> 13) & 0x0007);	//Significand	(3 bits)
}

//Convert a half-float to a float  (scalar)
float half2float(float16 in_half)
{
	short mantissa = (in_half & 0x7C00) >> 10;
	
	if (mantissa == 0)
		return 0;
	
	if (mantissa == 0x001F)
	{
		if (in_half & 0x8000)
			return -INFINITY;
		return INFINITY;
	}
	
	mantissa += 127 - 16;
	
	half_bits hb;
	hb.s[0] =		(in_half & 0x8000)			//Sign bit		(1 bits)
				|	((mantissa << 7) & 0x7F80)	//Mantissa		(8 bits)
				|	((in_half >> 3) & 0x007F);	//Significand	(7 bits)
	hb.s[1] =		(in_half << 13) & 0xFFFF;	//Significand	(16 bits)
	
	return hb.f;
}



//Output the data in a very legible way to stdout
void half2bin(float16 in_half)
{
	int x;
	for (x=15; x>=0; x--)
	{
		printf("%i", (in_half >> x) & 0x0001);
		if (x==15 || x == 10) printf(" ");
	}
}

void float2bin(float in_float)
{
	half_bits hb;
	hb.f = in_float;
	
	int x;
	for (x=31; x>=0; x--)
	{
		printf("%i", (hb.i >> x) & 0x0001);
		if (x==31 || x == 23) printf(" ");
	}
}
