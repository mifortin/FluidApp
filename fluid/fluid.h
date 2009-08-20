/*
 *  fluid.h
 *  FluidApp
 */

#ifndef FLUID_H
#define FLUID_H

#include "lua.h"
#include "error.h"
#include "net.h"
#include "protocol.h"

//Data organized in memory so that it's coherent for advection (all advection is
//based on floats)
typedef struct fluid fluid;

fluid *fluidCreate(int in_width, int in_height);

void fluidAdvance(fluid *in_f);

#endif

