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
	
	//Min/max in task list (always incrementing)
	atomic_int32	m_min, m_max;
	
	//------------------------------------------------------ NOT USED AT RUNTIME
	
	//Max number of tasks
	int				m_nMaxTasks;
};


//Frees the coherence engine
void mpCFree(void *in_o)
{
	mpCoherence *o = (mpCoherence*)in_o;
	if (o->r_tasks)		free(o->r_tasks);
}


//Creates a new coherence engine
mpCoherence *mpCCreate(int in_data, int in_tasks, int in_cache)
{
	errorAssert(in_data > 0, error_flags, "Need positive number of data sets");
	errorAssert(in_tasks > 0, error_flags, "Need positive number of tasks");
	errorAssert(in_cache > 0, error_flags, "Need positive number of caches");
	
	mpCoherence *o = x_malloc(sizeof(mpCoherence), mpCFree);
	memset(o, 0, sizeof(mpCoherence));
	
	o->r_tasks = malloc(sizeof(mpCoTask) * in_tasks);
	errorAssert(o->r_tasks != NULL, error_memory,
							"Failed allocating %i bytes for tasks",
							sizeof(mpCoTask) * in_tasks);
	
	o->m_nMaxTasks = in_tasks;
	o->m_nData = in_data;
	
	return o;
}


//Adds a task to the coherence engine
void mpCTaskAdd(mpCoherence *o, int in_fn, int in_depStart, int in_depEnd,
				int in_depLeft)
{
	errorAssert(in_depStart <= in_depEnd, error_flags,
						"Start must be less than or equal to end");
	errorAssert(in_depLeft >= 0, error_flags,
						"DepLeft must be greater or equal to zero");
	
	int rowToAddAt = AtomicExtract(o->m_nTasks);
	errorAssert(rowToAddAt < o->m_nMaxTasks, error_memory, "Too many tasks!!");
	
	if (in_depLeft)
		o->r_tasks[rowToAddAt].description.data.depend = DEPEND_INORDER;
	else
		o->r_tasks[rowToAddAt].description.data.depend = DEPEND_ANY_0;
	
	o->r_tasks[rowToAddAt].description.data.function = in_fn;
	
	if (in_depEnd < 0)	in_depEnd = 0;
	o->r_tasks[rowToAddAt].description.data.bottom = in_depEnd;
	
	errorAssert(AtomicCompareAndSwapInt(o->m_nTasks, rowToAddAt, rowToAddAt+1),
					error_thread, "Failed to atomically increment/modify row");
}
