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
	//Pointers to current buffers
	field *r_velocityX;
	field *r_velocityY;
	field *r_density;
	field *r_pressure;
	
	//Temporary buffers used as needed (we want to minimize memory footprint
	//to maximize cache usage)
	
	int m_curField;
	
	//Used for working...
	mpCoherence *r_coherence;
	
	//Used to stall the system until an iteration completes
	mpQueue		*r_blocker;
};



#endif
