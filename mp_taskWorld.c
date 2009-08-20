/*
 *  mp_taskWorld.c
 *  FluidApp
 */

#include "mpx.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>

#define MESSAGE_QUIT		100
#define MESSAGE_ACTION		200

//This is an array of `communications'.  They are placed within an mpQueue
//when the contents are used, and read from an mpQueue to use.  There is
//one per mpQueue entry.
typedef struct
{
	int message;		//What to do...
	mpTaskFn fn;		//The function...
	void *data;			//The data...
} mpTaskWorldCommunication;

typedef struct mpTaskWorld mpTaskWorld;
struct mpTaskWorld
{
	int m_workers;				//Number of workers that we have...

	mpQueue *r_sendQueue;		//Queue to send data...
	mpQueue *r_receiveQueue;	//Queue to receive data...
	mpTaskWorldCommunication *r_comms;	//Queue communication data
	
	//All the pthreads...
	pthread_t *rr_threads;		//All that threading...
};

mpTaskWorld *g_mpTaskWorld = NULL;

//Task engine
void *mpTaskEngine(void *in_o)
{
	//Task engine just loops until it receives the desired signal.
	x_try
		while (1)
		{
			mpTaskWorldCommunication *cur = mpQueuePop(in_o);
			switch(cur->message)
			{
				case MESSAGE_QUIT:
					return NULL;
					
				case MESSAGE_ACTION:
					cur->fn(cur->data);
					break;
			}
			
			mpQueuePush(g_mpTaskWorld->r_receiveQueue, cur);
		}
	x_catch(err)
		printf("mpTaskEngine failed: %s\n", errorMsg(err));
	x_finally
	
	return NULL;
}

void mpFree(void *in_o)
{
	//Only one instance, so this is easy!
	errorAssert(g_mpTaskWorld == in_o, error_specify, "Hmm, there's a bug here");
	
	//Wait for all of the task engines to complete....
	int i;
	for (i=0; i<g_mpTaskWorld->m_workers; i++)
	{
		mpTaskWorldCommunication *cur = mpQueuePop(g_mpTaskWorld->r_receiveQueue);
		cur->message = MESSAGE_QUIT;
		mpQueuePush(g_mpTaskWorld->r_sendQueue, cur);
	}
	
	for (i=0; i<g_mpTaskWorld->m_workers; i++)
	{
		x_pthread_join(g_mpTaskWorld->rr_threads[i]);
	}
	
	free(g_mpTaskWorld->rr_threads);
	x_free(g_mpTaskWorld->r_sendQueue);
	x_free(g_mpTaskWorld->r_receiveQueue);
	free(g_mpTaskWorld->r_comms);
}

void mpInit(int in_workers)
{
	errorAssert(g_mpTaskWorld == NULL, error_create, "mpInit called twice!");
	errorAssert(in_workers > 0, error_flags, "Need at least one worker");

	g_mpTaskWorld = x_malloc(sizeof(mpTaskWorld), mpFree);
	
	g_mpTaskWorld->m_workers = in_workers;
	g_mpTaskWorld->r_sendQueue = mpQueueCreate(in_workers * 4);
	g_mpTaskWorld->r_receiveQueue = mpQueueCreate(in_workers * 4);
	
	g_mpTaskWorld->rr_threads = malloc(sizeof(pthread_t*)*in_workers);
	
	g_mpTaskWorld->r_comms = malloc(sizeof(mpTaskWorldCommunication)*in_workers*4);
	
	int i;
	
	for (i=0; i<in_workers*4; i++)
	{
		mpQueuePush(g_mpTaskWorld->r_receiveQueue, g_mpTaskWorld->r_comms+i);
	}
	
	for (i=0; i<in_workers; i++)
	{
		x_pthread_create(&g_mpTaskWorld->rr_threads[i], NULL, mpTaskEngine,
							g_mpTaskWorld->r_sendQueue);
	}
}

void mpTerminate()
{
	x_free(g_mpTaskWorld);
	g_mpTaskWorld = NULL;
}


void mpTaskLaunch(mpTaskFn in_task, void *in_obj)
{
	mpTaskWorldCommunication *cur = mpQueuePop(g_mpTaskWorld->r_receiveQueue);
	cur->message = MESSAGE_ACTION;
	cur->fn = in_task;
	cur->data = in_obj;
	mpQueuePush(g_mpTaskWorld->r_sendQueue, cur);
}


int mpTaskFlood(mpTaskFn in_task, void *in_obj)
{
	int i;
	for (i=0; i<g_mpTaskWorld->m_workers; i++)
	{
		mpTaskWorldCommunication *cur = mpQueuePop(g_mpTaskWorld->r_receiveQueue);
		cur->message = MESSAGE_ACTION;
		cur->fn = in_task;
		cur->data = in_obj;
		mpQueuePush(g_mpTaskWorld->r_sendQueue, cur);
	}
	
	return g_mpTaskWorld->m_workers;
}
