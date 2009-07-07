/*
 *  mp_coherence.c
 *  FluidApp
 */

#include "mpx.h"

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

#define DEPEND_INORDER	255			//Dependency that is in order. (left->right)
#define DEPEND_ANY_0	0			//Index to list of dependencies

//Task to apply for start-middle-end.
typedef union mpCoDescription mpCoDescription
{
	atomic_int32	atom;
	struct
	{
		uint8_t			function;	//Function we're working with (ref below)
		uint8_t			bottom;		//Number of tiles we need (at least 1)
		uint8_t			depend;		//Dependency rules
		uint8_t			workFlag;	//Value means either 'did it' or 'pending
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
	//A list of all the tasks.  r_progress simply states how far down the
	//list we have gotten...
	mpCoTask		*r_tasks;
};
