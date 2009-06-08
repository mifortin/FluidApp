/*
 *  mp.h
 *  FluidApp
 */

#ifndef MP_H
#define MP_H

#include "error.h"
#include <pthread.h>

////////////////////////////////////////////////////////////////////////////////
//File dealing with the general multiprocessing issues.  Namely, a wrapper
//to protect pthread_mutexes at the moment.  Later on, multi-machine
//abstractions may be dumped here...
//
//	Note the error-processing overhead.  This assumes locks are used for
//	substantial amounts of work.  For the rest, there are atomic instructions.

typedef struct mpMutex mpMutex;

mpMutex *mpMutexCreate();

void mpMutexLock(mpMutex *in_m);
void mpMutexUnlock(mpMutex *in_m);


////////////////////////////////////////////////////////////////////////////////
//	The Queue - a way for threads to communicate by queing up messages.
//		Essentially the core of a basic message-passing API.
//
//		Each worker thread as a queue, as well as the main thread.  This allows
//		for easy communication.
//
//		Note - stalls may occur if these queues get filled to the brim.
//				At the moment, everything is done using pthreads, however
//				spinlocks might do a better job...
typedef struct mpQueue mpQueue;

mpQueue *mpQueueCreate(int maxSize);

void mpQueuePush(mpQueue *in_q, void *in_dat);
void *mpQueuePop(mpQueue *in_q);


////////////////////////////////////////////////////////////////////////////////
//	An updated version of the used threading paradigm to better suit the cell
//	processor.  What we have are 'n' worker threads.  Each of these threads
//	has some work to do.  The number is determined at initialization of the
//	system.
//
//	Exception handling is safe across an arbitrary number of threads since
//	data used for exceptions is stored within the stack.
typedef struct mpTaskWorld mpTaskWorld;

//Initialize core MP - essentially the task world as an opaque global...
//	Call mpTerminate to join all threads...
void mpInit(int in_workers);		//# of worker threads
void mpTerminate();


////////////////////////////////////////////////////////////////////////////////
//This is a port of the FXTaskSequence to C.  It's very annoying to do,
//however this will simplify things in the long run.  (as long as I
//can keep all these libraries seperate enough!)
typedef struct mpTaskSet mpTaskSet;

typedef error*(*mpTaskSetOnEndTask)(mpTaskSet *tsk, void *in_obj);
typedef error*(*mpTaskSetStart)(mpTaskSet *tsk, void *in_obj);

//Create the object...
//	Remember to synchronize the data!
mpTaskSet *mpTaskSetCreate(int in_nTasks, void *in_obj, 
						   mpTaskSetOnEndTask in_init,
						   mpTaskSetStart in_task, error **out_err);

//Launch as many threads as requested...
error *mpTaskSetLaunch(mpTaskSet *in_ts);

//All threads call this function when they complete a task.  Then once all
//threads are done, then they are released for the next task.
error *mpTaskSetSync(mpTaskSet *in_ts);

//Wait for all of the tasks to complete by joining them...
error *mpTaskSetJoin(mpTaskSet *in_ts);

//Critical sections... (for the threads)
error *mpTaskSetEnterCriticalSection(mpTaskSet *in_ts);
error *mpTaskSetLeaveCriticalSection(mpTaskSet *in_ts);


////////////////////////////////////////////////////////////////////////////////
//	PThread utilities / wrappers...
void x_pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*arg), void *arg);
void *x_pthread_join(pthread_t thread);

void x_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
void x_pthread_mutex_lock(pthread_mutex_t *mutex);
void x_pthread_mutex_unlock(pthread_mutex_t *mutex);

void x_pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
void x_pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
void x_pthread_cond_signal(pthread_cond_t *cond);

#endif
