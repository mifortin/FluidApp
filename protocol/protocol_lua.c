/*
 *  protocol_lua.c
 *  FluidApp
 */

#include "protocol_pvt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lauxlib.h"

#include "memory.h"

error *protocolLuaHandleData(protocol *in_proto,
							 void *in_pvt,
							 int in_size,
							 void *in_data)
{	
	protocolLua *pvt = (protocolLua*)in_pvt;
	
	char *cData = (char*)in_data;
	if (cData[in_size-1] != '\0')
		return errorCreate(NULL, error_net, "Host did not NULL-terminate data");
	
	error *e = NULL;
	mpMutexLock(pvt->r_lock);
	
	error *err = NULL;
	printf("Lua got script: %s\n", (char*)in_data);
	switch(luaL_loadstring(pvt->m_lua, (char*)in_data))
	{
		case LUA_ERRMEM:
			err= errorCreate(NULL, error_memory, "Lua out of memory: %s",
							   lua_tostring(pvt->m_lua, -1));
			break;
			
		case LUA_ERRSYNTAX:
			mpMutexUnlock(pvt->r_lock);
			e = protocolStringSend(in_proto, "\nLua syntax error: ");
			if (e) return e;
			e = protocolStringSend(in_proto, lua_tostring(pvt->m_lua,-1));
			return e;
	}
	
	if (err != NULL)
	{
		mpMutexUnlock(pvt->r_lock);
		return err;
	}
	
	switch(lua_pcall(pvt->m_lua, 0, 0, 0))
	{
		case LUA_ERRRUN:
			mpMutexUnlock(pvt->r_lock);
			e = protocolStringSend(in_proto, "\nLua runtime error: ");
			if (e) return e;
			e = protocolStringSend(in_proto, lua_tostring(pvt->m_lua,-1));
			return e;
			
		case LUA_ERRMEM:
			err = errorCreate(NULL, error_memory, "Lua out of memory: %s",
							   lua_tostring(pvt->m_lua, -1));
			break;
			
		case LUA_ERRERR:
			err = errorCreate(NULL, error_script,
							   "Lua error from error handler: %s",
							   lua_tostring(pvt->m_lua, -1));
			break;
	}
	
	if (err != NULL)
	{
		mpMutexUnlock(pvt->r_lock);
	}
	
	mpMutexUnlock(pvt->r_lock);
	
	return NULL;
}


void protocolLuaFree(void *in_o)
{
	protocolLua *in_proto = (protocolLua*)in_o;
	x_free(in_proto->r_lock);
}


protocolLua *protocolLuaCreate(protocol *in_proto, lua_State *in_lua,
							   mpMutex *in_lock, error **out_error)
{	
	protocolLua *toRet = x_malloc(sizeof(protocolLua), protocolLuaFree);
	if (toRet == NULL)
	{
		*out_error = errorCreate(NULL, error_memory,
								 "Failed allocating lua protocol extension");
		return NULL;
	}
	
	toRet->r_lock = in_lock;
	x_retain(toRet->r_lock);
	toRet->m_lua = in_lua;
	
	error *err;
	if (err = protocolAdd(in_proto, 'luae', toRet, protocolLuaHandleData))
	{
		x_free(toRet);
		*out_error = errorReply(err, error_specify,
								 "Failed registering luae protocol");
		return NULL;
	}
		
	return toRet;
}


error *protocolLuaSend(protocol *in_proto, const char *in_script)
{
	int size = strlen(in_script)+1;
	
	if (size > protocolMaxSize(in_proto))
		return errorCreate(NULL, error_flags,
				   "Too much script for protocol (got %i bytes for total %i)",
						   size, protocolMaxSize(in_proto));
	
	return protocolSend(in_proto, 'luae', size, in_script);
}
