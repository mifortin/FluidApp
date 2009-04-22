/*
 *  mp_mutex.c
 *  FluidApp
 */

#include "mp.h"
#include "memory.h"
#include <pthread.h>
#include <errno.h>

struct mpMutex
{
	pthread_mutex_t r_mutex;
};

void mpMutexDealloc(void *in_o)
{
	mpMutex *in_m = (mpMutex*)in_o;
	
	pthread_mutex_destroy(&in_m->r_mutex);
}


mpMutex *mpMutexCreate(error **outError)
{
	mpMutex *toRet = x_malloc(sizeof(mpMutex), mpMutexDealloc);
	if (toRet == NULL)
	{
		*outError = errorCreate(NULL, error_memory, "Failed allocating for mutex");
		return NULL;
	}
	
	pthread_mutex_t init = PTHREAD_MUTEX_INITIALIZER;
	toRet->r_mutex = init;
	
	return toRet;
}


error *mpMutexCodes(int in_code)
{
	switch(in_code)
	{
		case 0:
			return NULL;
		
		case EDEADLK:
			return errorCreate(NULL, error_thread, "Mutex: A dead-lock has occured");
			
		case EINVAL:
			return errorCreate(NULL, error_thread, "Mutex: Invalid value");
			
		case EPERM:
			return errorCreate(NULL, error_thread, "Mutex: Attempt to unlock unlocked mutex");
			
		default:
			return errorCreate(NULL, error_thread, "Mutex: Unknown error");
	}
}


error *mpMutexLock(mpMutex *in_m)
{
	return mpMutexCodes(pthread_mutex_lock(&in_m->r_mutex));
}


error *mpMutexUnlock(mpMutex *in_m)
{
	return mpMutexCodes(pthread_mutex_unlock(&in_m->r_mutex));
}
