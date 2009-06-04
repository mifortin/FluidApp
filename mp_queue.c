/*
 *  mp_queue.c
 *  FluidApp
 */

#include "mpx.h"
#include "memory.h"

#include <pthread.h>

struct mpQueue
{
	int m_readerPos;				//The current position in the stack of the queue
	int m_queueSize;				//Size (must be dynamic or else deadlocks may occur)
	
	void **r_queueData;				//Pointer to queue data (appended after)
	
	pthread_mutex_t		m_mutex;	//Simple mutex that we use
	pthread_cond_t		m_readCond;	//Condition (starved for instructions)
	pthread_cond_t		m_writeCond;//Condition (waiting for worker(s) to catch up)
};


void mpQueueFree(void *in_o)
{
	mpQueue *in_q = (mpQueue*)in_o;
	x_free(in_q->r_queueData);
	
	pthread_mutex_destroy(&in_q->m_mutex);
	pthread_cond_destroy(&in_q->m_readCond);
	pthread_cond_destroy(&in_q->m_writeCond);
}


mpQueue *mpQueueCreate(int maxSize)
{
	mpQueue *toRet = x_malloc(sizeof(mpQueue), mpQueueFree);
	
	toRet->r_queueData = x_malloc(sizeof(void*) * maxSize, mpQueueFree);
	toRet->m_queueSize = maxSize;
	
	toRet->m_readerPos = 0;
	
	x_pthread_mutex_init(&toRet->m_mutex, NULL);
	x_pthread_cond_init(&toRet->m_readCond, NULL);
	x_pthread_cond_init(&toRet->m_writeCond, NULL);
	
	return toRet;
}


void mpQueuePush(mpQueue *in_q, void *in_dat)
{
}


void *mpQueuePop(mpQueue *in_q)
{
	return NULL;
}
