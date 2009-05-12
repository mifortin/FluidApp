/*
 *  halfV.c
 *  FluidApp
 */

#include <stdio.h>

#include "half.h"
#include "memory.h"

struct halfV
{
	//Sizes of respective buffers (in # of elements)
	int f32_buffSize;
	int f16_buffSize;
	
	//Current location
	int curEntry;
	
	//State booleans
	int curState;
	
	//Pointers to the buffers
	float *f32_buff;
	float16 *f16_buff;
};


void halfVFree(void*in_v)
{ }


halfV *halfVCreate()
{
	halfV *toRet = x_malloc(sizeof(halfV), halfVFree);
	if (toRet == NULL)
	{
	}
	
	return NULL;
}
