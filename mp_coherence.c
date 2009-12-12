/*
 *  mp_coherence.c
 *  FluidApp
 */

#include "mpx.h"
#include "memory.h"
#include "error.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//#define COHERENCE_MUTEX

#ifdef COHERENCE_MUTEX

#define CoAtomicExtract(x)					(x)
#define CoAtomicCompareAndSwapInt(d,o,n)	((d = n) || 1)
#define CoAtomicAdd32Barrier(d,v)			(d = d + v)

#define co_pthread_mutex_unlock(m)
#define co_pthread_mutex_lock(m)

#define co_pthread_mutex_unlock_all(m)		/*printf("UNLOCK %i\n",__LINE__);*/x_pthread_mutex_unlock(m)
#define co_pthread_mutex_lock_all(m)		x_pthread_mutex_lock(m)/*;printf("LOCK %i\n",__LINE__);*/

#else

#define CoAtomicAdd32Barrier(d,v)			AtomicAdd32Barrier(d,v)
#define CoAtomicExtract(x)					AtomicExtract(x)
#define CoAtomicCompareAndSwapInt(d,o,n)	AtomicCompareAndSwapInt(d,o,n)

#define co_pthread_mutex_unlock(m)			x_pthread_mutex_unlock(m)
#define co_pthread_mutex_lock(m)			x_pthread_mutex_lock(m)

#define co_pthread_mutex_unlock_all(m)
#define co_pthread_mutex_lock_all(m)

#endif

//Union defining how much progress has been made
typedef union mpCoProgress mpCoProgress;
union mpCoProgress
{
	atomic_int32	atom;			//Variable to manipulate atomically
	struct
	{
		uint16_t		completed;	//The number of completed items
		uint16_t		working;	//Currently working on item
	} data;
};

#define DEPEND_INORDER	0			//Dependency that is in order. (left->right)
#define DEPEND_ANY_0	1			//Index to list of completed dependencies

//Task to apply for start-middle-end.
typedef union mpCoDescription mpCoDescription;
union mpCoDescription
{
	atomic_int32	atom;
	struct
	{
		uint16_t		depend;		//Dependency flags
		uint8_t			function;	//Function we're working with (ref below)
		uint8_t			bottom;		//Number of tiles we need (at least 1)
	} data;
};

//Union to send status messages across...
typedef union mpCoMessage mpCoMessage;
union mpCoMessage
{
	int	msg;
	struct
	{
		uint16_t	tid;		//We can obtain fn from tid
		uint16_t	task;		//We need task
	} data;
};

//Task for memory-coherent operations.
typedef struct mpCoTask mpCoTask;
struct mpCoTask
{
	mpCoProgress	progress;
	mpCoDescription	description;
} __attribute__((aligned(8)));


//Stores the progress of each of the tasks.  Tasks are organized on
//a grid.  Dependencies are organized based upon locations upon the grid.
struct mpCoherence
{	
	//-------------------------------------------------------- USED ALL THE TIME
	//A list of all the tasks.
	mpCoTask		*r_tasks;
	
	//Number of tasks
	atomic_int32	m_nTasks;
	
	//Amount of data each task must process
	int				m_nData;
	
	//Cache size + start  (cache is small "region")
	int				m_cacheSize;
	atomic_int32	m_nCacheStart;
	
	//Min/max in task list (always incrementing)
	atomic_int32	m_min, m_max;
	
	//------------------------------------------------------ NOT USED AT RUNTIME
	
	//Max number of tasks
	int				m_nMaxTasks;
	
	//Number of blocking threads...
	atomic_int32	m_nBlocking;
	
	//Mutex and conditional
	pthread_mutex_t	m_mutex;
	mpQueue			*r_q;
};


//Frees the coherence engine
void mpCFree(void *in_o)
{
	mpCoherence *o = (mpCoherence*)in_o;
	if (o->r_tasks)		free(o->r_tasks);
	if (o->r_q)			x_free(o->r_q);
	
	pthread_mutex_destroy(&o->m_mutex);
}


//Creates a new coherence engine
mpCoherence *mpCCreate(int in_data, int in_tasks, int in_cache)
{
	errorAssert(in_data > 0, error_flags, "Need positive number of data sets");
	errorAssert(in_tasks > 0, error_flags, "Need positive number of tasks");
	errorAssert(in_cache > 0, error_flags, "Need positive number of caches");
	
	mpCoherence *o = x_malloc(sizeof(mpCoherence), mpCFree);
	memset(o, 0, sizeof(mpCoherence));
	
	x_pthread_mutex_init(&o->m_mutex, NULL);
	
	o->r_tasks = malloc(sizeof(mpCoTask) * in_tasks);
	memset(o->r_tasks, 0, sizeof(mpCoTask) * in_tasks);
	errorAssert(o->r_tasks != NULL, error_memory,
							"Failed allocating %i bytes for tasks",
							sizeof(mpCoTask) * in_tasks);
	
	o->m_nMaxTasks = in_tasks;
	o->m_nData = in_data;
	o->m_cacheSize = in_cache;
	o->r_q = mpQueueCreate(2);		//Nice temp value
	
	return o;
}


void mpCReset(mpCoherence *o)
{
	o->m_nTasks = 0;
	o->m_min = 0;
	o->m_max = 0;
	o->m_nBlocking = 0;
	o->m_nCacheStart = 0;
	mpQueueClear(o->r_q);
}


//Adds a task to the coherence engine
void mpCTaskAdd(mpCoherence *o, int in_fn, int in_depStart, int in_depEnd,
				int in_depLeft)
{
	co_pthread_mutex_lock_all(&o->m_mutex);
								
	errorAssert(in_depStart <= in_depEnd, error_flags,
						"Start must be less than or equal to end");
	errorAssert(in_depLeft >= 0, error_flags,
						"DepLeft must be greater or equal to zero");
	
	int rowToAddAt = CoAtomicExtract(o->m_nTasks);
	errorAssert(rowToAddAt < o->m_nMaxTasks, error_memory, "Too many tasks!!");
	
	if (in_depLeft)
		o->r_tasks[rowToAddAt].description.data.depend = DEPEND_INORDER;
	else
		o->r_tasks[rowToAddAt].description.data.depend = DEPEND_ANY_0;
	
	o->r_tasks[rowToAddAt].description.data.function = in_fn;
	
	o->r_tasks[rowToAddAt].progress.data.completed = 0;
	o->r_tasks[rowToAddAt].progress.data.working = 0;
	
	if (in_depEnd < 0)	in_depEnd = 0;
	o->r_tasks[rowToAddAt].description.data.bottom = in_depEnd;
	
	errorAssert(CoAtomicCompareAndSwapInt(o->m_nTasks, rowToAddAt, rowToAddAt+1),
				error_thread, "Failed to atomically increment/modify row");
	
	co_pthread_mutex_unlock_all(&o->m_mutex);
}


int mpCTaskObtainPvt(mpCoherence *o, int *out_tid, int *out_fn, int *out_tsk,
									int x)
{
	mpCoProgress tmp;
	tmp.atom = CoAtomicExtract(o->r_tasks[x].progress.atom);

	if ((tmp.data.completed == tmp.data.working
		&& tmp.data.completed != o->m_nData) ||
		(o->r_tasks[x].description.data.depend == DEPEND_ANY_0
		 && tmp.data.working != o->m_nData))
	{
		int cacheStart = CoAtomicExtract(o->m_nCacheStart);
		if (tmp.data.working+1 < cacheStart)
		{
			int cs2 = cacheStart - o->m_nData + o->m_cacheSize;
			
			if (tmp.data.working+1 >= cs2)
			{
				return 0;
			}
		}

		if (x != 0)
		{
			//Make sure that we are obeying any dependencies before
			//continuing!
			mpCoProgress tM1;
			tM1.atom = CoAtomicExtract(o->r_tasks[x-1].progress.atom);
			
			int prevLimit = tmp.data.working + o->r_tasks[x].description.data.bottom;
			if (prevLimit >= o->m_nData)
				prevLimit = o->m_nData-1;
			
			if (tM1.data.completed
				<= prevLimit)
			{
				return 0;
			}
		}

		mpCoProgress t2;
		t2.atom = tmp.atom;
		int work = t2.data.working;
		t2.data.working++;
		//printf("OBTAINING: %i %i %i\n", x, o->r_tasks[x].description.data.function, work);
		if (CoAtomicCompareAndSwapInt(o->r_tasks[x].progress.atom,
									tmp.atom, t2.atom))
		{
			//printf("GIVEN: %i %i %i\n", x, o->r_tasks[x].description.data.function, work);
			if (t2.data.working > cacheStart + o->m_cacheSize)
				CoAtomicCompareAndSwapInt(o->m_nCacheStart, cacheStart, cacheStart+1);
			*out_tid = x;
			*out_fn = o->r_tasks[x].description.data.function;
			*out_tsk = work;
			
			//printf("ASSIGNED: %i %i %i\n", x, o->r_tasks[x].description.data.function, work);
			return 1;
		}
		else
		{
			return 0;
		}
	}
	
	return 0;
}


void mpCTaskObtain(mpCoherence *o, int *out_tid, int *out_fn, int *out_tsk)
{
	co_pthread_mutex_lock_all(&o->m_mutex);
	
	//Scan from min->max for a task to obtain...  (loop a bit before blocking)
	int last = CoAtomicExtract(o->m_nTasks);

	int min = CoAtomicExtract(o->m_min);
	int max = CoAtomicExtract(o->m_max)+1;
	if (max >= last)
		max = last-1;
	int x;
	for (x=min; x<=max; x++)
	{
		if (mpCTaskObtainPvt(o, out_tid, out_fn, out_tsk, x))
		{
			//printf("OBTAINED: %i %i %i\n", *out_tid, *out_fn, *out_tsk);
			co_pthread_mutex_unlock_all(&o->m_mutex);
			return;
		}
	}
	
	//Done?
	mpCoProgress tmp;
	tmp.atom = CoAtomicExtract(o->r_tasks[last-1].progress.atom);
	if (tmp.data.completed == o->m_nData)
	{
		*out_tid = -1;
		int v;
		do {
			v = CoAtomicExtract(o->m_nBlocking);
		} while (!CoAtomicCompareAndSwapInt(o->m_nBlocking, v, -100));
		co_pthread_mutex_unlock_all(&o->m_mutex);
		if (v >= 0)
		{
			mpQueuePushInt(o->r_q, 0xFFFFFFFF);
		}
		return;
	}
	
	//BLOCK HERE (use pthread conditions to not waste CPU)!
	co_pthread_mutex_lock(&o->m_mutex);
		CoAtomicAdd32Barrier(o->m_nBlocking, 1);
	co_pthread_mutex_unlock(&o->m_mutex);
	co_pthread_mutex_unlock_all(&o->m_mutex);
	*out_tid = mpQueuePopInt(o->r_q);
	if (*out_tid != 0xFFFFFFFF)
	{
		mpCoMessage msg;
		msg.msg = *out_tid;
		
		*out_tid = msg.data.tid;
		*out_fn = o->r_tasks[msg.data.tid].description.data.function;
		*out_tsk = msg.data.task;
		//printf("WAIT_FOR_TASK: %i %i %i\n", *out_tid, *out_fn, *out_tsk);
	}
	else
	{
		mpQueuePushInt(o->r_q, 0xFFFFFFFF);		//Unlock next thread...
		*out_tid = -1;
	}
}

void mpCTaskComplete(mpCoherence *o, int in_tid, int in_fn, int in_tsk,
									 int *out_tid, int *out_fn, int *out_tsk)
{
	//printf("COMPLETED: %i %i %i\n", in_tid, in_fn, in_tsk);
	//Mark the task as completed
	co_pthread_mutex_lock_all(&o->m_mutex);
	
	mpCoProgress tmp;
	tmp.atom = CoAtomicExtract(o->r_tasks[in_tid].progress.atom);
	int min = CoAtomicExtract(o->m_min);
	int max = CoAtomicExtract(o->m_max);
	
	mpCoDescription desc;
	desc.atom = o->r_tasks[in_tid].description.atom;
	
	if (desc.data.depend == DEPEND_ANY_0)
	{
		while (tmp.data.completed != in_tsk)		//Spin until task completes
		{
			co_pthread_mutex_unlock_all(&o->m_mutex);
			co_pthread_mutex_lock_all(&o->m_mutex);
			tmp.atom = CoAtomicExtract(o->r_tasks[in_tid].progress.atom);
		}
	}
	
	mpCoProgress newVal;
	newVal.atom = tmp.atom;
	newVal.data.completed = newVal.data.completed+1;
	
	if (tmp.data.working == o->m_nData)
	{
		if (min == in_tid)
		{
			if (desc.data.depend == DEPEND_ANY_0)
			{
				if (CoAtomicCompareAndSwapInt(o->m_min, min, min+1))
				{
					while (!CoAtomicCompareAndSwapInt(o->m_nCacheStart,
										CoAtomicExtract(o->m_nCacheStart), 0));
				}
			}
			else
			{
				errorAssert(CoAtomicCompareAndSwapInt(o->m_min, min, min+1),
							error_thread, "Failed incrementing min!");
				while (!CoAtomicCompareAndSwapInt(o->m_nCacheStart,
							CoAtomicExtract(o->m_nCacheStart), 0));
			}
		}
		min++;
	}
	
	if (in_tid == max)
	{
		if (desc.data.depend == DEPEND_ANY_0)
		{
			CoAtomicCompareAndSwapInt(o->m_max, max, max+1);
		}
		else
		{
			errorAssert(CoAtomicCompareAndSwapInt(o->m_max, max, max+1),
						error_thread, "Failed incrementing max!");
		}
	}
	
	errorAssert(tmp.data.completed+1 == tmp.data.working
				|| desc.data.depend == DEPEND_ANY_0,
					error_thread, "Already marked as completed (%i %i)!",
						tmp.data.completed+1, tmp.data.working);

	if (desc.data.depend == DEPEND_ANY_0)
	{
		while (!CoAtomicCompareAndSwapInt(o->r_tasks[in_tid].progress.atom,
										tmp.atom, newVal.atom))
		{
			tmp.atom = CoAtomicExtract(o->r_tasks[in_tid].progress.atom);
			newVal.atom = tmp.atom;
			
			newVal.data.completed = newVal.data.completed + 1;
		}
	}
	else
	{
		errorAssert(CoAtomicCompareAndSwapInt(o->r_tasks[in_tid].progress.atom,
										tmp.atom, newVal.atom),
						error_thread, "Only 1 thread per task!!!");
	}
	
	int totalTasks = CoAtomicExtract(o->m_nTasks);
	max++;
	if (max >= totalTasks)
		max = totalTasks-1;
	
	int s = in_tid;
	if (s < min)	s = min;
	if (s > max)	s = max;
	int e = s+1;
	//if (e < min)	e = min;
	if (e > max)	e = max;
	
	int x;
	for (x=e; x>=s; x--)
	{
		if (mpCTaskObtainPvt(o, out_tid, out_fn, out_tsk, x))
			break;
	}
	if (x < s)
	{
		co_pthread_mutex_unlock_all(&o->m_mutex);
		mpCTaskObtain(o, out_tid, out_fn, out_tsk);
		co_pthread_mutex_lock_all(&o->m_mutex);
		//printf("BEST GUESS FAILED %i - %i ==> %i - %i\n", in_tid, in_tsk,
		//									*out_tid, *out_tsk);
	}

	//Signal another thread to awake
	int v = CoAtomicExtract(o->m_nBlocking);
	if (v > 0)
	{
		//printf("Recuperating from cache misses!\n");
		co_pthread_mutex_lock(&o->m_mutex);
		
		v = CoAtomicExtract(o->m_nBlocking);
		if (v == 0)
		{
			co_pthread_mutex_unlock(&o->m_mutex);
			co_pthread_mutex_unlock_all(&o->m_mutex);
			return;
		}
		mpCoMessage data;
		data.data.tid = *out_tid;
		data.data.task = *out_tsk;
		mpQueuePushInt(o->r_q,data.msg);
		//printf("FORWARDING: %i %i %i\n", *out_tid, *out_fn, *out_tsk);
		CoAtomicCompareAndSwapInt(o->m_nBlocking, v, v-1);
		
		
		for (x=max;x>=min;x--)
		{
			v = CoAtomicExtract(o->m_nBlocking);
			if (v == 0)
				break;
		
			int tmp_tid, tmp_fn, tmp_tsk;
			if (mpCTaskObtainPvt(o, &tmp_tid, &tmp_fn, &tmp_tsk, x))
			{
				//printf("FORWARDING: %i %i %i\n", tmp_tid, tmp_fn, tmp_tsk);
				CoAtomicCompareAndSwapInt(o->m_nBlocking, v, v-1);
				data.data.tid = tmp_tid;
				data.data.task = tmp_tsk;
				mpQueuePushInt(o->r_q,data.msg);
			}
		}
		
		
		co_pthread_mutex_unlock(&o->m_mutex);
		co_pthread_mutex_unlock_all(&o->m_mutex);
		
		//Finally worry about this thread...
		mpCTaskObtain(o, out_tid, out_fn, out_tsk);
	}
	else
	{
		co_pthread_mutex_unlock_all(&o->m_mutex);
	}
	//printf("RETURNING: (%i %i %i) %i %i %i\n",
	//	   in_tid, in_fn, in_tsk,
	//	   *out_tid, *out_fn, *out_tsk);
}
