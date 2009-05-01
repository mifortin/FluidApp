/*
 *  fluid_cpuCore.h
 *  FluidApp
 */

#ifndef FLUID_CPUCORE_H
#define FLUID_CPUCORE_H

/**
 *	This particular piece of code does the general stuff.  AKA, this is the
 *	library that the CPU code calls to get the work done.
 *
 *	There are raging debates in my head as to allow Objective-C or not.  However,
 *	the level of the runtime needed to use Objective-C on other platforms is
 *	not exagerated - so I'll use it...
 */


#include <libkern/OSAtomic.h>

/**
	The reason that there is a stream description as a C struct is that looping
	over this is quicker than always doing OBJ-C calls all over the place.
 
	The other option is to add the obj-c to array bridge within the functions,
	which adds a useless layer of complexity to this code...
 */
#define FSCPU_Type_Character2D		1
#define FSCPU_Type_Float2D			2
typedef struct
{
	//The data that we're dealing with
	union
	{
		float *f;
		unsigned char *c;
	} data;
	
	//The default values for said data (needed for collisions)
	union  {
		float *f;
		unsigned char *c;
	} defaults;
	
	//How to iterate over the data
	int strideX;
	int strideY;
	
	//How much data do we have?
	int width;
	int height;
	
	//How many components do we have?
	//	- Specify and use in all cases
	int components;
	
	//Type type of stream that this is... (int for alignment)
	int type;
} fluidStreamDesc;

/** Utility function to quickly/easily create one of these structures... */
fluidStreamDesc fluidStreamDescMakeCharacter2D(unsigned char *data, unsigned char*defaults, int strideX, int strideY, int width, int height, int components);
fluidStreamDesc fluidStreamDescMakeFloat2D(float *data, float*defaults, int strideX, int strideY, int width, int height, int components);

#endif
