/*
 *  Test_Pipeline.c
 *  FluidApp
*
 */

#include "memory.h"
#include <stdlib.h>
#include <math.h>

#define DS (16)
#define BLOCK 8

#define TYPE long

//Normally there are multiple integer units - let's see if we can show that
//they exist...
TYPE simpleSum(TYPE n)
{
	TYPE i = 0;
	for (TYPE x=0; x<n; x++)
		i+=x;
	
	return i;
}

TYPE parallelSum(TYPE n)
{
	TYPE a=0, b=0, c=0, d=0;
	
	TYPE x;
	for (x=0; x<n-4; x+=4)
	{
		a+=x;
		b+=x+1;
		c+=x+2;
		d+=x+3;
	}
	
	for (;x<n;x++)
		a+=x;
	
	return a+b+c+d;
}

void testPipeline()
{	
	int j;
	for (j=0; j<10;j++)
	{
		TYPE k;
		double t1 = x_time();
		k = parallelSum(1000000000);
		printf("Time: Parallel: %f = %li \n", x_time() - t1, k);
		
		
		t1 = x_time();
		k = simpleSum(1000000000);
		printf("Time: Simple: %f = %li \n", x_time() - t1, k);
	}
}
