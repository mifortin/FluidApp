/*
 *  fluid_spu.h
 *  
 */

//Maximum width of a piece of data within a buffer...
#define MAX_WIDTH		1024

#define COMMAND_NOTHING		' '			//Do nothing with memory
#define COMMAND_DELAY		'd'			//Read while doing work
#define COMMAND_STALL		's'			//Read, start working after data fetched
#define COMMAND_WRITE		'w'			//Write the data whilee working

#define CMD_NOOP			0x00		//Just want to do some DMA requests

#define CMD_PRESSURE		0x01		//Middle pressure-case
#define CMD_PRESSURE_APPLY	0x04		//Applies the pressure

#define CMD_VISCOSITY		0x0A		//Normal viscosity



typedef struct {
	//Addresses of data to load...
	void *velocityX;
	void *velocityY;
	void *input1;
	void *output1;
	void *pressure;
	
	//Arguments to the function
	float timestep;
	float alpha, beta;
	
	//Width (in elements)
	int width;
	
	//Start and length of data reading
	int start;
	int count;
	
	//maximum size...
	int height;
	
	//Padding...
	unsigned char pad[64-48];
	
} fluid_context __attribute__ ((aligned(128)));
