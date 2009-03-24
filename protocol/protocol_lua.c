/*
 *  protocol_lua.c
 *  FluidApp
 */

#include "protocol_pvt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

error *protocolLuaHandleData(protocol *in_proto,
							 void *in_pvt,
							 int in_size,
							 void *in_data)
{
	protocolLua *pvt = (protocolLua*)in_pvt;
	
	char *cData = (char*)in_data;
	if (cData[in_size-1] != '\0')
		return errorCreate(NULL, error_net, "Host did not NULL-terminate data");
	
	if (pthread_mutex_lock(&pvt->m_lock) != 0)
		return errorCreate(NULL, error_thread, "Failed locking lua lock");
	
	error *err = NULL;
	switch(luaL_loadstring(pvt->m_lua, (char*)in_data))
	{
		case LUA_ERRMEM:
			err= errorCreate(NULL, error_memory, "Lua out of memory: %s",
							   lua_tostring(pvt->m_lua, -1));
			break;
			
		case LUA_ERRSYNTAX:
			err = errorCreate(NULL, error_script, "Syntax error in lua script: %s",
							lua_tostring(pvt->m_lua, -1));
			break;
	}
	
	if (err != NULL)
	{
		pthread_mutex_unlock(&pvt->m_lock);
		return err;
	}
	
	switch(lua_pcall(pvt->m_lua, 0, 0, 0))
	{
		case LUA_ERRRUN:
			err = errorCreate(NULL, error_script, "Lua runtime error: %s",
							   lua_tostring(pvt->m_lua, -1));
			break;
			
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
		pthread_mutex_unlock(&pvt->m_lock);
		return err;
	}
	
	if (pthread_mutex_unlock(&pvt->m_lock) != 0)
		return errorCreate(NULL, error_thread, "Failed unlocking lua lock");
	
	return NULL;
}


protocolLua *protocolLuaCreate(protocol *in_proto, lua_State *in_lua,
							   pthread_mutex_t in_lock, error **out_error)
{	
	protocolLua *toRet = malloc(sizeof(protocolLua));
	if (toRet == NULL)
	{
		*out_error = errorCreate(NULL, error_memory,
								 "Failed allocating lua protocol extension");
		return NULL;
	}
	
	toRet->m_lock = in_lock;
	toRet->m_lua = in_lua;
	
	error *err;
	if (err = protocolAdd(in_proto, 'luae', toRet, protocolLuaHandleData))
	{
		free(toRet);
		*out_error = errorReply(err, error_specify,
								 "Failed registering luae protocol");
		return NULL;
	}
		
	return toRet;
}


void protocolLuaFree(protocolLua *in_proto)
{
	if (in_proto)
	{
		pthread_mutex_destroy(&in_proto->m_lock);
		free(in_proto);
	}
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
