/*
 *  protocol_string.c
 *  FluidApp
 */

#include "protocol_pvt.h"
#include "memory.h"


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
	for (i=1; i<=nParams; i++)
	{
		protocolStringSend(ps, lua_tostring(in_lua, i));
						   
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
	x_retain(in_objHandler);
	
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
}
