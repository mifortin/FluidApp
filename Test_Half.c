/*
 *  Test_Half.c
 *  FluidApp
 */

#include "half.h"
#include <math.h>
#include <stdio.h>

//Test support for half-floats...
void testHalfFloat()
{
	//First, test normalized positive values...
	printf(" Testing floats:\n");
	int x;
	for (x=0; x<100; x++)
	{
		float v = (float)x / 100.0f;
		float e = half2float(float2half(v));
		if (fabs(v-e) > 0.001f)
		{
			printf("    %2.4f -> %2.4f e %2.4f\n", v, e, fabs(v-e));
			printf("    S:");
				float2bin(v);
				printf("\n");
			printf("    H:");
				half2bin(float2half(v));
				printf("\n");
			printf("    R:");
				float2bin(e);
				printf("\n");
			
			//For debugging reasons...
			e = half2float(float2half(v));
		}
	}
}
