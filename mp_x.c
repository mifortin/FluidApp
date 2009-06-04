/*
 *  mp_x.c
 *  FluidApp
 *
 */

#include "mpx.h"
#include "memory.h"

#include <errno.h>

void x_pthread_raise(int errValue, char *context)
{
	switch (errValue)
	{
		case 0: break;
		
		case EDEADLK:
			x_raise(errorCreate(NULL, error_thread, "%s: A dead-lock has occured", context));
			
		case EINVAL:
			x_raise(errorCreate(NULL, error_thread, "%s: Invalid attribute specified", context));
			
		case ENOMEM:
			x_raise(errorCreate(NULL, error_thread, "%s: Not enough memory", context));
			
		case EAGAIN:
			x_raise(errorCreate(NULL, error_thread, "%s: Can't create now... try later?", context));
			
		case EPERM:
			x_raise(errorCreate(NULL, error_thread, "%s: Attempt to unlock unlocked mutex", context));
			
		default:
			x_raise(errorCreate(NULL, error_thread, "%s: Unknown error", context));
	}
}


void x_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
	x_pthread_raise(pthread_mutex_init(mutex, attr), "Mutex Create");
}

void x_pthread_mutex_lock(pthread_mutex_t *mutex)
{
	x_pthread_raise(pthread_mutex_lock(mutex), "Mutex Lock");
}

void x_pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	x_pthread_raise(pthread_mutex_unlock(mutex), "Mutex Unlock");
}



void x_pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
	x_pthread_raise(pthread_cond_init(cond, attr), "Condition");
}
