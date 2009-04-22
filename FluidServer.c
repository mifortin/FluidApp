/*
 *  FluidServer.c
 *  FluidApp
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "net.h"
#include "lua.h"
#include "lualib.h"
#include "fluid.h"
#include "protocol.h"
#include "error.h"
#include "memory.h"

volatile int tmp = 0;

#define LUA_HEAP	(64*1024)

pthread_mutex_t m;

void * luaAlloc (void *ud,
                             void *ptr,
                             size_t osize,
                             size_t nsize)
{
	if (nsize == 0)
	{
		free(ptr);
		return NULL;
	}
	
	if (osize != 0 && nsize < LUA_HEAP)
		return ptr;
	else if (osize != 0)
		return NULL;
	
	if (nsize < LUA_HEAP)
		return malloc(LUA_HEAP);
	else
		return NULL;
}


int onConnect(void *d, netServer *in_vr, netClient *in_remote)
{	
	lua_State *L = lua_newstate(luaAlloc, NULL);
	luaopen_base(L);
	luaopen_string(L);
	luaopen_math(L);
	
	error *pError = NULL;
	mpMutex *mtx = mpMutexCreate(&pError);
	protocol *p = protocolCreate(in_remote, 1024*4, &pError);
	protocolLua *pl = protocolLuaCreate(p, L, mtx, &pError);
	protocolFloatCreate(p, 10, L, mtx, &pError);
	
	if (p == NULL)
	{
		printf("Failed creating protocol: %s\n", errorMsg(pError));
		x_free(pError);
		return 0;
	}
	
	for (;;)
	{
		mpMutexLock(mtx);
		
		lua_getglobal(L, "eventLoop");
		if (lua_isfunction(L, -1) && !lua_isnone(L,-1) && !lua_isnil(L,-1))
		{
			lua_pcall(L,0,0,0);
		}
		else
			lua_pop(L,1);
		
		mpMutexUnlock(mtx);
	}
	
	x_free(mtx);
	x_free(pl);
	x_free(p);
	lua_close(L);
	
	return 0;
}


int main(int argc, char *argv[])
{	
	printf("Fluid Server Launching\n");
	pthread_mutex_init(&m, NULL);
	
	error *err = NULL;
	netServer *server = netServerCreate("2048", NETS_TCP, NULL, onConnect, &err);
	if (server)
	{
		printf("Server Launched\n");
		
		x_free(server);
		
		pthread_mutex_lock(&m);
		printf("Value of tmp: %i\n", tmp);
		pthread_mutex_unlock(&m);
	}
	else
		printf("Failed launching: %s\n", errorMsg(err));
	
	fflush(stdout);
	return 0;
}
