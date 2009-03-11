/*
 *  fluid_pvt.h
 *  FluidApp
 */

#ifndef FLUID_PVT_H
#define FLUID_PVT_H

#include "fluid.h"
#include "field.h"

struct fluid
{
	//	0 - velocity
	//	1 -
	//	2 - pressure
	//	3 - free surfaces
	//	4 - collisions
	//	5 - 
	//	6 - position/repos
	//	7 -
	field *fluidData[2];
};

#endif
