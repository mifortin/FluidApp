/*
 *  fluid_jacobi.h
 *  FluidApp
 */

#ifndef FLUID_JACOBI_H
#define FLUID_JACOBI_H

#include "error.h"
#include "mpx.h"
#include "fluid_cpuCore.h"

//Basic implementation of matrix solver
error *fluidJacobiPressure(int in_nIterations, mpTaskSet *in_seq,
						  int32_t *atomicY,
						  fluidStreamDesc velocity,
						  fluidStreamDesc collision,
						  fluidStreamDesc pressure[],
						  int vectorize);

error *fluidJacobiDiffusion(int in_nIterations,
						   mpTaskSet *in_seq, int32_t *atomicY,
						   fluidStreamDesc *v1, fluidStreamDesc *v2,
						   fluidStreamDesc *collision,
						   float viscocity, float timestep);

//Note that accuracy must be an array with as many elements as iterations.
error *fluidJacobiPressure_WithErrorLog(int in_nIterations, mpTaskSet *in_seq,
						  int32_t *atomicY,
						  fluidStreamDesc velocity,
						  fluidStreamDesc collision,
						  fluidStreamDesc pressure[],
						  float *out_accuracy,
						  int vectorize);

error *fluidJacobiDiffusion_WithErrorLog(int in_nIterations,
						   mpTaskSet *in_seq, int32_t *atomicY,
						   fluidStreamDesc *v1, fluidStreamDesc *v2,
						   fluidStreamDesc *collision,
						   float viscocity, float timestep,
						   float *out_accuracy);

#endif

