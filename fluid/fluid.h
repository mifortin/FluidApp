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

fluid *fluidCreate(protocol *in_proto, lua_State *in_ls, char *in_globName,
					int in_width, int in_height, error **out_err);
void fluidFree(fluid *in_fluid);

#endif

