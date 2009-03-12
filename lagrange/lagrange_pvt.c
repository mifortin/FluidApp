/*
 *  lagrange_pvt.c
 *  FluidApp
 */

#include "lagrange_pvt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

lagrange *lagrangeCreate(lua_State *in_lua, const char *in_luaGlobName,
						 int in_maxParticles, error **out_error)
{
	lagrange *toRet;
	*out_error = NULL;
	
	if (in_maxParticles <= 0)
	{
		*out_error = errorCreate(NULL, error_flags, "Need at least a particle");
	}
	
	if (in_maxParticles%16 != 0)
	{
		*out_error = errorCreate(NULL, error_flags, "Number of particles needs to
													be multiples of 16");
	}
	
	toRet = malloc(sizeof(lagrange));
	if (toRet == NULL)
	{
		*out_error = errorCreate(NULL, error_memory,
									"Unable to allocate struct lagrange");
		return NULL;
	}
	
	if (in_lua)
	{
		toRet->m_name = malloc(sizeof(in_luaGlobName)+1);
		if (toRet->m_name == NULL)
		{
			lagrangeFree(toRet);
			return NULL;
		}
		strcpy(toRet->m_name, in_luaGlobName);
		
		lua_newtable(in_lua);
		lua_setfield(in_lua, LUA_GLOBALSINDEX, in_luaGlobName);
		toRet->m_lua = in_lua;
	}
	
	return toRet;
}


void lagrangeFree(lagrange *in_l)
{
	if (in_l)
	{
		//Destroy the object within lua...
		if (in_l->m_lua)
		{
			lua_pushnil(in_l->m_lua);
			lua_setfield(in_l->m_lua, LUA_GLOBALSINDEX, in_l->m_name);
		}
		
		//Free everything else...
		if (in_l->m_fPositions)		free(in_l->m_fPositions);
		if (in_l->m_fVelocities)	free(in_l->m_fVelocities);
		if (in_l->m_fAccelerations)	free(in_l->m_fAccelerations);
		if (in_l->m_fDistances)		free(in_l->m_fDistances);
		if (in_l->m_nSurroundings)	free(in_l->m_nSurroundings);
		
		if (in_l->m_name)			free(in_l->m_name);
		
		free(in_l);
	}
}
