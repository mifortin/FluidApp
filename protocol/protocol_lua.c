/*
 *  protocol_lua.c
 *  FluidApp
 */

#include "protocol_pvt.h"

#include <stdio.h>
#include <stdlib.h>


error *protocolLuaHandleData(protocol *in_proto,
							 void *in_pvt,
							 int in_size,
							 void *in_data)
{
	
	
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
	
	
		
	return toRet;
}

void protocolLuaFree(protocolLua *in_proto)
{
	if (in_proto)
	{
		pthread_mutex_destroy(in_proto->m_lock);
		free(in_proto);
	}
}
