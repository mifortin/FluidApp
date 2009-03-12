/*
 *  lagrange.h
 *  FluidApp
 */

#ifndef LAGRANGE_H
#define LAGRANGE_H

#include "error.h"
#include "lua.h"

//Defines a particle system designed to simulate fluids in 2D...
//typedef struct lagrange_particle lagrange_particle;
typedef struct lagrange lagrange;

lagrange *lagrangeCreate(lua_State *in_lua, const char *in_luaGlobName,
						 int in_maxParticles, error **out_error);

void lagrangeFree(lagrange *in_l);

#endif
