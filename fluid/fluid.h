/*
 *  fluid.h
 *  FluidApp
 */

#ifndef FLUID_H
#define FLUID_H

#include "error.h"
#include "net.h"
#include "field.h"

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

//Buoyancy
void fluidSetGravityVector(fluid *f, float x, float y);
void fluidSetGravityMagnitude(fluid *f, float v);
void fluidSetTemperatureMagnitude(fluid *f, float v);

//Set up vorticity (2 modes of computation)
void fluidVorticityQuick(fluid *f);
void fluidVorticityAccurate(fluid *f);

//Set up free surfaces
void fluidFreeSurfaceNone(fluid *f);
void fluidFreeSurfaceSimple(fluid *f);

//Enables the timers for the next frame of
//simulation
void fluidEnableTimers(fluid *f);
void fluidDisableTimers(fluid *f);

float fluidAdvectionTime(fluid *f);
float fluidPressureTime(fluid *f);
float fluidViscosityTime(fluid *f);
float fluidVorticityTime(fluid *f);
float fluidThreadSchedulerTime(fluid *f);

//Video-output functions
field *fluidVideoOut(fluid *in_f);
void fluidVideoVelocityOut(fluid *in_f, field *in_dest);

//Video-input functions
void fluidVideoBlendIn(fluid *in_f, field *in_ch, float in_s);
void fluidVelocityBlendIn(fluid *in_f, field *in_ch, float in_s);

//Select accelerator
void fluidEnableCPU(fluid *in_f);
void fluidEnableCL(fluid *in_f);


//FluidMessenger is a seperate object that dispatches messages to the fluid,
//servers, and others to handle communication (either from command line or from
//remote applications)
typedef struct fluidMessenger fluidMessenger;
typedef void(*fluidMessengerHandler)(void *o, fieldMsg *msg);

fluidMessenger *fluidMessengerCreate(fluid *in_f);

// 1 is success!
int fluidMessengerHandleMessage(fluidMessenger *fm, fieldMsg *msg);

void fluidMessengerAddHandler(fluidMessenger *fm, fluidMessengerHandler h,
								void *ptr);

#endif

