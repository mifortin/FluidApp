/*
 *  fluid.h
 *  FluidApp
 */

#ifndef FLUID_H
#define FLUID_H

#include "error.h"
#include "net.h"
#include "field.h"

//All of the data that is needed by external code.
//	(note - external code should request more densities for additional
//	advected parameters)
typedef struct fluidTask fluidTask;
struct fluidTask
{
	field *velX;		//Current x + y velocities
	field *velY;
	
	field *dens;		//Current densities
};

typedef void(*fluidTaskFn)(void *in_o, int in_row, fluidTask *tsk);

//Data organized in memory so that it's coherent for advection (all advection is
//based on floats)
typedef struct fluid fluid;

fluid *fluidCreate(int in_width, int in_height);

void fluidAdvance(fluid *in_f);

field *fluidDensity(fluid *in_f);
field *fluidMovedDensity(fluid *in_f);
field *fluidVelocityX(fluid *in_f);
field *fluidVelocityY(fluid *in_f);

//Allows the user of the system to set the viscosity...
void fluidSetViscosity(fluid *f, float in_v);
void fluidSetVorticity(fluid *f, float in_v);
void fluidSetTimestep(fluid *f, float in_v);

//Set up fading...
void fluidSetDensityFade(fluid *f, float in_v);
void fluidSetVelocityFade(fluid *f, float in_v);

//Set up free surfaces
void fluidFreeSurfaceNone(fluid *f);
void fluidFreeSurfaceSimple(fluid *f);

//Add in custom forces to the simulation (just after advection)
void fluidAddExternalTask(fluid *f,				//Instance to the fluid sim
						  void *o,				//Instance to the ext obj
						  fluidTaskFn fn,		//The task object
						  int32_t *in_cnt);		//Atomically incremented when timed.

//Enables the timers for the next frame of
//simulation
void fluidEnableTimers(fluid *f);
void fluidDisableTimers(fluid *f);

float fluidAdvectionTime(fluid *f);
float fluidPressureTime(fluid *f);
float fluidViscosityTime(fluid *f);
float fluidVorticityTime(fluid *f);

#endif

