/*
 *  mp_taskWorld.c
 *  FluidApp
 */

#include "mpx.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>

struct mpTaskWorld
{
	int m_workers;				//Number of workers that we have...

	mpQueue *r_sendQueue;		//Queue to send data...
	
	//All the pthreads...
	pthread_t *rr_threads;		//All that threading...
};


//Task engine
void *mpTaskEngine(void *in_o)
{
	//Task engine just loops until it receives the desired signal.

	return NULL;
}


mpTaskWorld *g_mpTaskWorld;

void mpFree(void *in_o)
{
	//Only one instance, so this is easy!
	errorAssert(g_mpTaskWorld == in_o, error_specify, "Hmm, there's a bug here");
	
	//Wait for all of the task engines to complete.... 
	//	(maybe I should send a quite message before waiting indefinitely on
	//	them)
	int i;
	for (i=0; i<g_mpTaskWorld->m_workers; i++)
	{
		x_pthread_join(g_mpTaskWorld->rr_threads[i]);
	}
	
	free(g_mpTaskWorld->rr_threads);
	x_free(g_mpTaskWorld->r_sendQueue);
}

void mpInit(int in_workers)
{
	errorAssert(g_mpTaskWorld == NULL, error_create, "mpInit called twice!");
	errorAssert(in_workers > 0, error_flags, "Need at least one worker");

	g_mpTaskWorld = x_malloc(sizeof(mpTaskWorld), mpFree);
	
	g_mpTaskWorld->m_workers = in_workers;
	g_mpTaskWorld->r_sendQueue = mpQueueCreate(in_workers * 4);
	
	g_mpTaskWorld->rr_threads = malloc(sizeof(pthread_t*)*in_workers);
	
	int i;
	for (i=0; i<in_workers; i++)
	{
		x_pthread_create(&g_mpTaskWorld->rr_threads[i], NULL, mpTaskEngine,
							g_mpTaskWorld->r_sendQueue);
	}
}

void mpTerminate()
{
	x_free(g_mpTaskWorld);
}

