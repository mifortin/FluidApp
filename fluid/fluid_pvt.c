/*
 *  fluid_pvt.c
 *  FluidApp
 */

#include "fluid_pvt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int fluid_advection_lua(lua_State *in_l)
{
	int n = lua_gettop(in_l);
	
	if (n != 2)
	{
		lua_pushstring(in_l, "advection takes two parameters");
		lua_error(in_l);
	}
	
	if (!lua_isuserdata(in_l,1))
	{
		lua_pushstring(in_l, "First parameter must be userdata");
		lua_error(in_l);
	}
	
	if (!lua_isstring(in_l,2))
	{
		lua_pushstring(in_l, "Second parameter must be a string");
		lua_error(in_l);
	}
	
	const char *c = lua_tostring(in_l, 2);
	if (strcmp(c, "stam") == 0)
	{
		fluid_advection_stam((fluid*)lua_touserdata(in_l,1));
	}
	
	return 0;
}


field *fluid_curField(fluid *in_f)
{
	return in_f->fluidData[in_f->m_curField];
}


field *fluid_destField(fluid *in_f)
{
	return in_f->fluidData[1-in_f->m_curField];
}


void fluid_swapField(fluid *in_f)
{
	in_f->m_curField = 1.0f - in_f->m_curField;
}


fluid *fluidCreate(lua_State *in_ls, char *in_globName,
					int in_width, int in_height, error **out_err)
{
	fluid *toRet = malloc(sizeof(fluid));
	
	if (toRet == NULL)
	{
		*out_err = errorCreate(NULL, error_memory,
									"Out of memory creating fluid object");
		return NULL;
	}
	
	memset(toRet, 0, sizeof(fluid));
	
	error *tmpError = NULL;
	toRet->fluidData[0] = fieldCreate(in_width, in_height, 8, &tmpError);
	if (toRet->fluidData[0] == NULL)
	{
		*out_err = errorReply(tmpError, error_create,
									"Failed creating fluid data");
		free(toRet);
		return NULL;
	}
	
	toRet->fluidData[1] = fieldCreate(in_width, in_height, 8, &tmpError);
	if (toRet->fluidData[1] == NULL)
	{
		*out_err = errorReply(tmpError, error_create,
									"Failed creating fluid data");
		fieldFree(toRet->fluidData[0]);
		free(toRet);
		return NULL;
	}
	
	if (in_ls)
	{
		toRet->m_szGlobName = malloc(strlen(in_globName)+1);
		strcpy(toRet->m_szGlobName, in_globName);
	
		//Setup lua...
		lua_newtable(in_ls);
		lua_setglobal(in_ls, in_globName);
		
		toRet->m_ls = in_ls;
	}
	
	return toRet;
}


void fluidFree(fluid *in_fluid)
{
	if (in_fluid)
	{
		if (in_fluid->fluidData[0])	fieldFree(in_fluid->fluidData[0]);
		if (in_fluid->fluidData[1]) fieldFree(in_fluid->fluidData[1]);
		
		if (in_fluid->m_ls)
		{
			lua_pushnil(in_fluid->m_ls);
			lua_setglobal(in_fluid->m_ls, in_fluid->m_szGlobName);
		}
		
		if (in_fluid->m_szGlobName)	free(in_fluid->m_szGlobName);
		
		free(in_fluid);
	}
}
