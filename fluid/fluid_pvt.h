/*
 *  fluid_pvt.h
 *  FluidApp
 */

#ifndef FLUID_PVT_H
#define FLUID_PVT_H

#include "fluid.h"
#include "field.h"

////////////////////////////////////////////////////////////////////////////////
//
//		Structure defining how we use the fluid
//
struct fluid
{
	//Pointers to current buffers
	field *r_velocityX;
	field *r_velocityY;
	field *r_density;
	field *r_pressure;
	
	field *r_tmpVelX;		//Temporary velocity X (advection work as Stam)
	field *r_tmpVelY;		//Temporary velocity Y (advection work as Stam)
	
	field *r_reposX;		//Used to accelerate advection (noticeable now
	field *r_reposY;		//							that data is in cache!)
	
	//Temporary buffers used as needed (we want to minimize memory footprint
	//to maximize cache usage)
	
	int m_curField;
	
	//Used for working...
	mpCoherence *r_coherence;
	
	//Used to stall the system until an iteration completes
	mpQueue		*r_blocker;
};

#define TIMESTEP	1.0f

////////////////////////////////////////////////////////////////////////////////
//
//			Useful methods (implemented elsewhere)
//
void fluid_advection_stam_velocity(fluid *in_f, int rowID);
void fluid_advection_mccormack_repos(fluid *in_f, int rowID);

#endif
