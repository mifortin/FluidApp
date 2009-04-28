/*
 *  memory.c
 *  FluidApp
 */

#include "memory.h"

#include <stdio.h>
#include <stdlib.h>

#include <libkern/OSAtomic.h>

typedef struct memory_pvt memory_pvt;
struct memory_pvt
{
	int32_t rc;							//The retain count...
	x_dealloc kill;						//Your guess is as good as mine...
} __attribute__ ((aligned(16)));

//Replacement allocator
void *x_malloc(int size, x_dealloc in_d)
{
	memory_pvt *toRet = malloc(size + sizeof(memory_pvt));
	toRet->rc = 1;
	toRet->kill = in_d;
	return toRet + 1;
}

//Replacment free function
void x_free(void *in_o)
{
	memory_pvt *r = (memory_pvt*)in_o;
	int nVal = OSAtomicAdd32Barrier(-1, &r[-1].rc);
	
	if (nVal == 0)
	{
		r[-1].kill(in_o);
		free(r-1);
	}
}

//Retain (extension)
void x_retain(void *in_o)
{
	memory_pvt *r = (memory_pvt*)in_o;
	OSAtomicAdd32Barrier(1, &r[-1].rc);
}