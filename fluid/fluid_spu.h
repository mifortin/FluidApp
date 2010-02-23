/*
 *  fluid_spu.h
 *  
 */

//Number of buffers that can be uploaded to a given core at any time
#define FLUID_BUFFERS	33

//Maximum width of a piece of data within a buffer...
#define MAX_WIDTH		1920

typedef struct {
	//Addresses of data to load...
	void *addresses[FLUID_BUFFERS];
	
	//What to do with these buffers...
	//	0xAABBBBBB
	//
	//	A =	00 - nothing
	//		01 - load (no pause!)	- priority 2	(Stalls if DMA not done at end)
	//		02 - load (paused!)		- priority 1
	//
	//	B =	Map arg N of command to a given buffer
	char commands[FLUID_BUFFERS];
	
	//Which command to run?
	//	'p'	= pressure
	char cmd;
} fluid_context __attribute__ ((aligned(128)));
