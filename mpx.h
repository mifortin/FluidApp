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

mpQueue *mpQueueCreate(int in_maxSize);

void mpQueuePush(mpQueue *in_q, void *in_dat);
void *mpQueuePop(mpQueue *in_q);

void mpQueuePushInt(mpQueue *in_q, int in_dat);
int mpQueuePopInt(mpQueue *in_q);


////////////////////////////////////////////////////////////////////////////////
//	Data forms the basis of optimizations for data-bound software.  As the
//	number of cores increases, the more "data-bound" the software becomes.
//
//	This is essentially a way to "organize" data.  Data is a container of
//	tasks.  Tasks depend upon other tasks.
//
//	There is a "fixed" amount of "data".  A "datum" is defined as a
//	set of memory that is worked upon by a task.  "datum" can be complex
//	and include multiple records.
//
//	Each "datum" also resides on seperate cache lines.
//
//	For a fluid simulation, there is at least 8*row "datum"
//
//	These datum are mapped to 8*row*(40+40+10) = 8*row*(90) tasks
//	(19 is an approximation for non-iterative tasks).
//
//	For each task, we have at least 2 "ready" tasks, 4 max.  Each ready task
//	gets it's counters reduced.  The one whose most memory is in cache and
//	all dependencies met gets executed.
//
typedef struct mpCoherence mpCoherence;

//Description of a function used for coherence operations
//	We're passed in the object and a 'cacheID'
//		cacheID identifies a computation on a given set of memory addresses.
typedef void(*mpCFn)(void *o, int cacheID);

//Creates a new coherence object.  This is seperate from the threading
//engine for simplicity.
//	data is the number of data sets to work with
//	tasks is the number of tasks that will be run (usually about 90-100)
//	cache is the amount of data that can be loaded into the proc's cache,
//	NOTE: values will need tuning...
mpCoherence *mpCCreate(int in_data, int in_tasks, int in_cache);



////////////////////////////////////////////////////////////////////////////////
//	An updated version of the used threading paradigm to better suit the cell
//	processor.  What we have are 'n' worker threads.  Each of these threads
//	has some work to do.  The number is determined at initialization of the
//	system.
//
//	Exception handling is safe across an arbitrary number of threads since
//	data used for exceptions is stored within the stack.

//Initialize core MP - essentially the task world as an opaque global...
//	Call mpTerminate to join all threads...
void mpInit(int in_workers);		//# of worker threads
void mpTerminate();

//Function for a task... (obj points to something passed in on launch that
//can be retained)
typedef void(*mpTaskFn)(void *in_obj);

//Start a new task.  Note - the main thread is the controller, and these are
//the workers.  A set number of workers exist from the start of the app
//	("optimal" number based on number of cores).
void mpTaskLaunch(mpTaskFn in_task, void *in_obj);


//Start a task parallelized over the number of cores.  Differs from mpTaskLaunch
//by setting up a queue entry for each core.  Useful when simple sync methods
//can be used (eg. atomic instructions and the like)
void mpTaskFlood(mpTaskFn in_task, void *in_obj);


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
//	Atomic utility / wrappers
#ifndef CELL

//Mac OS specific atomics
#include <libkern/OSAtomic.h>

#define atomic_int32	int32_t

#define AtomicAdd32Barrier(x,y)				OSAtomicAdd32Barrier((y), &(x))
#define AtomicCompareAndSwapInt(d, o, n)	OSAtomicCompareAndSwap32Barrier((o), (n), &(d))
#define AtomicCompareAndSwapPtr(d, o, n)	OSAtomicCompareAndSwapPtrBarrier((o), (n), &(d))
#define AtomicCompareInt(d, o)				OSAtomicCompareAndSwap32Barrier((o), (o), &(d))
#define AtomicExtract(d)					OSAtomicAdd32Barrier(0, &(d))

#else

#define atomic_int32	int32_t

//Other atomics  (GCC 4.2+)
#define AtomicAdd32Barrier(x,y)				__sync_add_and_fetch(&(x), (y))
#define AtomicCompareAndSwapInt(d, o, n)	__sync_bool_compare_and_swap(&(d), (o), (n))
#define AtomicCompareAndSwapPtr(d, o, n)	__sync_bool_compare_and_swap(&(d), (o), (n))
#define AtomicCompareInt(d, o)				__sync_bool_compare_and_swap(&(d), (o), (o))
#define AtomicExtract(d)					__sync_add_and_fetch(&(d), 0)

#endif


////////////////////////////////////////////////////////////////////////////////
//	PThread utilities / wrappers...
void x_pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*arg), void *arg);
void *x_pthread_join(pthread_t thread);

void x_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
void x_pthread_raise(int errValue, char *context);
#define x_pthread_mutex_lock(X) { int e = pthread_mutex_lock(X); if (e) x_pthread_raise(e,"Mutex Lock");}

#define x_pthread_mutex_unlock(X) { int e = pthread_mutex_unlock(X); if (e) x_pthread_raise(e,"Mutex Unlock");}

void x_pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
void x_pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
void x_pthread_cond_signal(pthread_cond_t *cond);

#endif
