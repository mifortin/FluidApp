/*
 *  protocol_float.c
 *  FluidApp
 */

#include "protocol_pvt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

//Simple way to store the data...
typedef struct protocolFloatPvt protocolFloatPvt;
struct protocolFloatPvt
{
	int index;
	float data;
};

error *protocolFloatHandleData(protocol *in_proto,
							 void *in_pvt,
							 int in_size,
							 void *in_data)
{
	protocolFloat *pf = (protocolFloat*)in_pvt;
	
	if (in_size != sizeof(protocolFloatPvt))
		return errorCreate(NULL, error_net, "Invalid size of input packet");
	
	protocolFloatPvt *dt = (protocolFloatPvt*)in_data;
	int index = ntohl(dt->index);
	float data = ntohl(dt->data);
	
	if (index < 0 || index >= pf->m_noElements)
		return errorCreate(NULL, error_net, "Invalid float element referenced");
	
	if (pthread_mutex_lock(&pf->m_dataLock) != 0)
		return errorCreate(NULL, error_thread, "Failed locking mutex");
	
	pf->r_elements[index] = data;
	
	if (pthread_mutex_unlock(&pf->m_dataLock) != 0)
		return errorCreate(NULL, error_thread, "Failed unlocking mutex");
	
	return NULL;
}


void protocolFloatFree(protocolFloat *in_f)
{
	if (in_f)
	{
		pthread_mutex_destroy(&in_f->m_dataLock);
		free(in_f->r_elements);
		free(in_f);
	}
}


int protocolFloatLuaGC(lua_State *in_lua)
{
	printf("LUA Float GC\n");
	
	protocolFloatFree(lua_touserdata(in_lua, 1));
	
	return 0;
}


int protocolFloatLuaGet(lua_State *in_lua)
{
	printf("LUA Float Get\n");
	
	error *err = NULL;
	
	lua_pushnumber(in_lua,
				   protocolFloatReceive(lua_touserdata(in_lua, 1),
										lua_tointeger(in_lua, 2),
										&err));
	
	if (err != NULL)
	{
		lua_pushstring(in_lua, errorMsg(err));
		lua_error(in_lua);
	}
	
	return 1;
}


int protocolFloatLuaSet(lua_State *in_lua)
{
	printf("LUA Float Set\n");
	
	error *err = NULL;
	
	err = protocolFloatSend(lua_touserdata(in_lua, 1),
							lua_tointeger(in_lua, 2),
							lua_tonumber(in_lua, 3));
	
	return 0;
}


protocolFloat *protocolFloatCreate(protocol *in_p,
								   int in_numElements,
								   lua_State *in_lua,
								   pthread_mutex_t in_luaLock,
								   error **out_error)
{
	if (in_numElements <= 0)
	{
		*out_error = errorCreate(NULL, error_create,
								 "Need positive number of elements");
		return NULL;
	}
	
	protocolFloat *toRet = malloc(sizeof(protocolFloat));
	if (toRet == NULL)
	{
		*out_error = errorCreate(NULL, error_memory,
								 "Failed allocating memory for float extensions");
		return NULL;
	}
	
	toRet->m_proto = in_p;
	toRet->m_luaLock = in_luaLock;
	toRet->m_noElements = in_numElements;
	toRet->r_elements = calloc(in_numElements,sizeof(float));
	if (toRet->r_elements == NULL)
	{
		free(toRet);
		*out_error = errorCreate(NULL, error_memory, "Need more memory!");
		return NULL;
	}
	
	if (pthread_mutex_init(&toRet->m_dataLock, NULL) != 0)
	{
		free(toRet->r_elements);
		free(toRet);
		*out_error = errorCreate(NULL, error_thread,
								 "Failed creating thread");
		return NULL;
	}
	
	if (pthread_mutex_lock(&toRet->m_luaLock) != 0)
	{
		protocolFloatFree(toRet);
		*out_error = errorCreate(NULL, error_thread, "Failed locking mutex");
		return NULL;
	}
	
	protocolFloat **ld = lua_newuserdata(in_lua, sizeof(protocolFloat*));
	*ld = toRet;
	
	lua_newtable(in_lua);
	
	lua_pushstring(in_lua, "__gc");
	lua_pushcfunction(in_lua, protocolFloatLuaGC);
	lua_settable(in_lua, -3);
	
	lua_pushstring(in_lua, "__index");
	lua_pushcfunction(in_lua, protocolFloatLuaGet);
	lua_settable(in_lua, -3);
	
	lua_pushstring(in_lua, "__newindex");
	lua_pushcfunction(in_lua, protocolFloatLuaSet);
	lua_settable(in_lua, -3);
	
	lua_setmetatable(in_lua, -2);
	
	lua_setglobal(in_lua, "net_float");
	
	if (pthread_mutex_unlock(&toRet->m_luaLock) != 0)
	{
		//Ensure a GC pass...
		lua_pushnil(in_lua);
		lua_setglobal(in_lua, "net_float");
		
		*out_error = errorCreate(NULL, error_thread, "Failed unlocking mutex");
		return NULL;
	}
	
	return toRet;
}


//These methods are for data access.  Lua calls these as well.
float protocolFloatReceive(protocolFloat *in_f, int in_eleNo,
						   error **out_err)
{
	if (in_eleNo < 0 || in_eleNo >= in_f->m_noElements)
	{
		*out_err = errorCreate(NULL, error_flags, "Index out of range");
		return 0;
	}
	
	int toRet;
	
	if (pthread_mutex_lock(&in_f->m_dataLock) != 0)
	{
		*out_err = errorCreate(NULL, error_thread, "Failed locking mutex");
		return 0;
	}
	
	toRet = in_f->r_elements[in_eleNo];
	
	if (pthread_mutex_unlock(&in_f->m_dataLock) != 0)
	{
		*out_err = errorCreate(NULL, error_thread, "Failed unlocking mutex");
	}
	
	return toRet;
}

error *protocolFloatSend(protocolFloat *in_f, int in_eleNo, float in_val)
{
	return NULL;
}
