/*
 *  mp.h
 *  FluidApp
 */

#ifndef MP_H
#define MP_H

#include "error.h"

//File dealing with the general multiprocessing issues.  Namely, a wrapper
//to protect pthread_mutexes at the moment.  Later on, multi-machine
//abstractions may be dumped here...
//
//	Note the error-processing overhead.  This assumes locks are used for
//	substantial amounts of work.  For the rest, there are atomic instructions.

typedef struct mpMutex mpMutex;

mpMutex *mpMutexCreate(error **out_error);

error *mpMutexLock(mpMutex *in_m);
error *mpMutexUnlock(mpMutex *in_m);


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

#endif
