/*
 *  fluid_pvt.c
 *  FluidApp
 */

#include "fluid_pvt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"


void fluidFree(void *in_o)
{
	fluid *o = (fluid*)in_o;
	
	if (o->r_coherence)		x_free(o->r_coherence);
	
	if (o->r_blocker)		x_free(o->r_blocker);
	
	if (o->r_pressure)		x_free(o->r_pressure);
	if (o->r_density)		x_free(o->r_density);
	if (o->r_velocityX)		x_free(o->r_velocityX);
	if (o->r_velocityY)		x_free(o->r_velocityY);
}


fluid *fluidCreate(int in_width, int in_height)
{
	fluid *toRet = x_malloc(sizeof(fluid), fluidFree);
	memset(toRet, 0, sizeof(fluid));
	
	toRet->r_velocityX = fieldCreate(in_width, in_height, 1);
	toRet->r_velocityY = fieldCreate(in_width, in_height, 1);
	toRet->r_density = fieldCreate(in_width, in_height, 1);
	toRet->r_pressure = fieldCreate(in_width, in_height, 1);
	
	toRet->r_blocker = mpQueueCreate(2);
	
	//NOTE: Make this configurable????
	toRet->r_coherence = mpCCreate(in_height, 128, 64);
	
	toRet->m_curField = 0;
	
	return toRet;
}


//Called on each processor to do a specified amount of work.
void fluidMP(void *in_o)
{
	fluid *o = (fluid*)in_o;
	mpCoherence *c = o->r_coherence;
	
	
	
	mpQueuePush(o->r_blocker, NULL);
}


//Called every frame to advance the fluid simulation...
void fluidAdvance(fluid *in_f)
{
	//Add in the basic fluid simulation as it was before - except with SIMPLE
	//boundary conditions

	//We just need to run the tasks that have already been setup...
	int spawned = mpTaskFlood(fluidMP, in_f);
	
	int i;
	for (i=0; i<spawned; i++)
		mpQueuePop(in_f->r_blocker);
	
	//Clear the scheduler (for the next pass)
	mpCReset(in_f->r_coherence);
}
