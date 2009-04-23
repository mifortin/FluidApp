/*
 *  protocol_string.c
 *  FluidApp
 */

#include "protocol_pvt.h"
#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

error *protocolStringHandleData(protocol *in_proto,
							   void *in_pvt,
							   int in_size,
							   void *in_data)
{
	protocolString *in_str = (protocolString*)in_pvt;
	
	//Ensure that the string is NULL terminated...
	char *tmp = (char*)in_data;
	
	if (tmp[in_size-1] != '\0')
	{
		return errorCreate(NULL, error_net, "We only accept NULL-terminated strings");
	}
	
	//Call our handler...
	return in_str->m_handler(in_str->r_handlerObject, (const char*)in_data);
}


void protocolStringFree(void *in_o)
{
	protocolString *in_s = (protocolString*)in_o;
	
	if (in_s->r_handlerObject)	x_free(in_s->r_handlerObject);
}


int protocolStringLuaGC(lua_State *in_lua)
{
	printf("LUA String GC\n");
	
	x_free(*((protocolString**)lua_touserdata(in_lua, 1)));
	
	return 0;
}

int protocolStringLuaCall(lua_State *in_lua)
{
	int nParams = lua_gettop(in_lua);
	
	protocolString *ps = *((protocolString**)lua_touserdata(in_lua, 1));
	
	int i;
	char tmpBuffer[16];
	for (i=1; i<=nParams; i++)
	{
		if (lua_isnil(in_lua, i))
			protocolStringSend(ps->m_proto, " (nil) " );
		else if (lua_isnone(in_lua, i))
			protocolStringSend(ps->m_proto, " (none) " );
		else if (lua_istable(in_lua, i))
			protocolStringSend(ps->m_proto, " (table) ");
		else if (lua_isboolean(in_lua, i))
		{
			if (lua_toboolean(in_lua, i))
				protocolStringSend(ps->m_proto, " yes ");
			else
				protocolStringSend(ps->m_proto, " no ");
		}
		else if (lua_isnumber(in_lua, i))
		{
			snprintf(tmpBuffer, sizeof(tmpBuffer), " %f ",
					 lua_tonumber(in_lua, i));
		}
		else if (lua_isstring(in_lua, i))
			protocolStringSend(ps->m_proto, lua_tostring(in_lua, i));
		else
			protocolStringSend(ps->m_proto, " (?) ");
	}
	
	return 0;
}


protocolString *protocolStringCreate(	protocol *in_p,
										lua_State *in_lua,
										mpMutex *in_luaLock,
										void *in_objHandler,
										protocolStringHandler in_handler,
										error **out_error)
{
	protocolString *toRet = x_malloc(sizeof(protocolString), protocolStringFree);
	if (toRet == NULL)
	{
		*out_error = errorCreate(NULL, error_create, "need more memory for allocation");
		return NULL;
	}
	
	toRet->m_handler = in_handler;
	toRet->r_handlerObject = in_objHandler;
	toRet->m_proto = in_p;
	x_retain(in_objHandler);
	
	*out_error = protocolAdd(in_p, 'str0', toRet, protocolStringHandleData);
	if (out_error)
		return NULL;
	
	
	if (in_lua || in_luaLock)
	{
		if (!in_lua || !in_luaLock)
		{
			x_free(toRet);
			*out_error = errorCreate(NULL, error_flags, "Lua needs lock and vice versa");
			return NULL;
		}
		
		*out_error = mpMutexLock(in_luaLock);
		if (*out_error) return NULL;
		
		lua_newtable(in_lua);
		
		lua_pushstring(in_lua, "__gc");
		lua_pushcfunction(in_lua, protocolStringLuaGC);
		lua_settable(in_lua, -3);
		
		lua_pushstring(in_lua, "__call");
		lua_pushcfunction(in_lua, protocolStringLuaCall);
		lua_settable(in_lua, -3);
		
		lua_setmetatable(in_lua, -2);
		
		lua_setglobal(in_lua, "net_log");
		
		*out_error = mpMutexUnlock(in_luaLock);
		if (*out_error) return NULL;
	}
	
	return toRet;
}


error *protocolStringSend(protocol *in_p, const char *in_string)
{
	int size = strlen(in_string)+1;
	
	if (size > protocolMaxSize(in_p))
		return errorCreate(NULL, error_flags,
						   "String too long! (got %i characters for a max of %i)",
						   size, protocolMaxSize(in_p));
	
	return protocolSend(in_p, 'str0', size, in_string);
}
