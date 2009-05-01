/*
 *  mp_task.c
 *  FluidApp
 */

#include "mpx.h"
#include <pthread.h>
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>

struct mpTaskSet
{
	//The current task.  We start at 0, each thread will block
	//when they finish a given task.
	int m_curTask;
	
	//The number of threads that are blocked.  Once this equals the number
	//of threads, it's reset, and everyone gets back to work...
	int m_numBlocked;
	
	//The number of threads.
	int m_numThreads;
	
	//The threads...
	pthread_t *r_threads;
	
	//The mutex
	pthread_mutex_t r_mutex;
	
	//The condition
	pthread_cond_t r_condition;
	
	//The dispatched
	void *m_obj;
	mpTaskSetStart m_dispatcher;
	mpTaskSetOnEndTask m_terminator;
};


//Frees the mutex
void mpTaskSetDealloc(void *in_o)
{
	mpTaskSet *in_t = (mpTaskSet*)in_o;
	
	pthread_mutex_destroy(&in_t->r_mutex);
	pthread_cond_destroy(&in_t->r_condition);
	
	if (in_t->r_threads)	free(in_t->r_threads);
}


//Create the object...
//	Remember to synchronize the data!
mpTaskSet *mpTaskSetCreate(int in_nTasks, void *in_obj, 
						   mpTaskSetOnEndTask in_init,
						   mpTaskSetStart in_task, error **out_err)
{
	mpTaskSet *toRet = x_malloc(sizeof(mpTaskSet), mpTaskSetDealloc);
	if (toRet == NULL)
	{
		*out_err = errorCreate(NULL, error_memory, "Out of memory");
		return NULL;
	}
	
	
	if (pthread_mutex_init(&toRet->r_mutex, NULL) != 0)
	{
		*out_err = errorCreate(NULL, error_thread, "Failed creating mutex");
		x_free(toRet);
		return NULL;
	}
	
	if (pthread_cond_init(&toRet->r_condition, NULL) != 0)
	{
		*out_err = errorCreate(NULL, error_thread, "Failed creating condition");
		x_free(toRet);
		return NULL;
	}
	
	toRet->r_threads = malloc(sizeof(pthread_t)*in_nTasks);
	if (toRet->r_threads == NULL)
	{
		x_free(toRet);
		*out_err = errorCreate(NULL, error_memory, "Out of memory");
		return NULL;
	}
	
	toRet->m_curTask = 0;
	toRet->m_numBlocked = 0;
	toRet->m_numThreads = in_nTasks;
	
	toRet->m_obj = in_obj;
	toRet->m_dispatcher = in_task;
	toRet->m_terminator = in_init;
	
	return toRet;
}


void *mpTaskSetThread(void *arg)
{
	mpTaskSet *in_ts = (mpTaskSet*)arg;
	
	return in_ts->m_dispatcher(in_ts, in_ts->m_obj);
}


//Launch as many threads as requested...
error *mpTaskSetLaunch(mpTaskSet *in_ts)
{
	int x;
	for (x=0; x<in_ts->m_numThreads; x++)
	{
		pthread_create(&in_ts->r_threads[x], NULL, mpTaskSetThread, in_ts);
	}
	
	return NULL;
}

//All threads call this function when they complete a task.  Then once all
//threads are done, then they are released for the next task.
error *mpTaskSetSync(mpTaskSet *in_ts)
{
	if (pthread_mutex_lock(&in_ts->r_mutex) != 0)
		return errorCreate(NULL, error_thread, "Failed locking mutex");

	error *toRet = NULL;

	int task = in_ts->m_curTask +1;
	in_ts->m_numBlocked = in_ts->m_numBlocked+1;
	
	if (in_ts->m_numBlocked == in_ts->m_numThreads)
	{
		(in_ts->m_curTask)++;
		in_ts->m_numBlocked =0;
		
		toRet = in_ts->m_terminator(in_ts, in_ts->m_obj);
		
		pthread_cond_broadcast(&in_ts->r_condition);
	}
	else
	{
		while (task != in_ts->m_curTask)
			pthread_cond_wait(&in_ts->r_condition, &in_ts->r_mutex);
	}

	if (pthread_mutex_unlock(&in_ts->r_mutex) != 0)
	{
		if (toRet) x_free(toRet);
		return errorCreate(NULL, error_thread, "Failed unlocking mutex");
	}

	return toRet;
}

//Wait for all of the tasks to complete by joining them...
error *mpTaskSetJoin(mpTaskSet *in_ts)
{
	error *toRet = NULL;
	
	int x;
	for (x=0; x<in_ts->m_numThreads;x++)
	{
		error *e;
		pthread_join(in_ts->r_threads[x], (void**)&e);
		
		if (e)
		{
			if (toRet)
				x_free(e);
			else
				toRet = e;
		}
	}
	
	return toRet;
}


error *mpTaskSetEnterCriticalSection(mpTaskSet *in_ts)
{
	if (pthread_mutex_lock(&in_ts->r_mutex) != 0)
		return errorCreate(NULL, error_thread, "Failed locking mutex");
	
	return NULL;
}

error *mpTaskSetLeaveCriticalSection(mpTaskSet *in_ts)
{
	if (pthread_mutex_unlock(&in_ts->r_mutex) != 0)
		return errorCreate(NULL, error_thread, "Failed unlocking mutex");
	
	return NULL;
}
