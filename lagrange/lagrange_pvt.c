/*
 *  lagrange_pvt.c
 *  FluidApp
 */

#include "lagrange_pvt.h"


lagrange *lagrangeCreate(lua_State *in_lua, const char *in_luaGlobName,
						 int in_maxParticles, error **out_error)
{
	lagrange *toRet;
	
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
