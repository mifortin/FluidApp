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

#endif
