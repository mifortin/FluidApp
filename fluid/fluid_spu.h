/*
 *  fluid_spu.h
 *  
 */

//Number of buffers that can be uploaded to a given core at any time
#define FLUID_BUFFERS	33

//Maximum width of a piece of data within a buffer...
#define MAX_WIDTH		1920

#define COMMAND_NOTHING		' '			//Do nothing with memory
#define COMMAND_DELAY		'd'			//Read while doing work
#define COMMAND_STALL		's'			//Read, start working after data fetched
#define COMMAND_WRITE		'w'			//Write the data whilee working

#define CMD_NOOP			0x00		//Just want to do some DMA requests

#define CMD_PRESSURE		0x01		//Middle pressure-case
#define CMD_PRESSURE_B		0x02		//Border-condition pressure
#define CMD_PRESSURE_APPLY	0x04		//Applies the pressure

#define CMD_VISCOSITY		0x0A		//Normal viscosity
#define CMD_VISCOSITY_B		0x0B		//viscosity border-condition



typedef struct {
	//Addresses of data to load...
	void *addresses[FLUID_BUFFERS];
	
	//Arguments to the function
	float timestep;
	float alpha, beta;
	
	//Width (in 4-4-byte floats)
	int width;
	
	//What to do with these buffers...
	//	0xAABBBBBB
	//
	//	A =	00 - nothing
	//		01 - load (no pause!)	- priority 2	(Stalls if DMA not done at end)
	//		02 - load (paused!)		- priority 1
	//		03 - write-out			- 
	//
	//	B =	Map arg N of command to a given buffer
	char commands[FLUID_BUFFERS];
	
	//Arguments (in order) passed to function...
	char args[FLUID_BUFFERS];
	
	//Which command to run?
	//	'p'	= pressure
	unsigned char cmd;
	
	//Padding...
	unsigned char pad[40];
	
} fluid_context __attribute__ ((aligned(16)));
