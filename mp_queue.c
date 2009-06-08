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
	
	int m_readers;					//Number of threads stalled on read condition
	int m_writers;					//Number of threads stalled on write condition
	
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
	x_pthread_mutex_lock(&in_q->m_mutex);
	
	while (in_q->m_queueSize == in_q->m_readerPos)
	{
		in_q->m_writers++;
		x_pthread_cond_wait(&in_q->m_writeCond, &in_q->m_mutex);
	}
	
	in_q->r_queueData[in_q->m_readerPos] = in_dat;
	in_q->m_readerPos++;
	
	if (in_q->m_readers > 0)
	{
		in_q->m_readers--;
		x_pthread_cond_signal(&in_q->m_readCond);
	}
	
	x_pthread_mutex_unlock(&in_q->m_mutex);
}


void *mpQueuePop(mpQueue *in_q)
{
	x_pthread_mutex_lock(&in_q->m_mutex);
	
	while (in_q->m_readerPos == 0)
	{
		in_q->m_readers++;
		x_pthread_cond_wait(&in_q->m_readCond, &in_q->m_mutex);
	}
	
	in_q->m_readerPos--;
	void *toRet = in_q->r_queueData[in_q->m_readerPos];
	
	if (in_q->m_writers > 0)
	{
		in_q->m_writers--;
		x_pthread_cond_signal(&in_q->m_writeCond);
	}
	
	x_pthread_mutex_unlock(&in_q->m_mutex);
	
	return toRet;
}
