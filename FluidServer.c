/*
 *  FluidServer.c
 *  FluidApp
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>

#include "net.h"
#include "lua.h"
#include "lualib.h"
#include "fluid.h"
#include "protocol.h"
#include "error.h"
#include "memory.h"
#include "field.h"
#include "mpx.h"

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


error *hndlr(void *o, const char *d)
{
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
	protocolStringCreate(p, L, mtx, mtx, hndlr, &pError);
	
	field *f = fieldCreate(p, 512, 512, 16, L, mtx, &pError);
	
	protocolSetReadyState(p);
	
	float *t = (float*)fieldDataLock(f, &pError);
	int x;
	for (x=0; x<512*512*16; x++)
		t[x] = 1;
		

	int y;
	for (x=0; x<512; x++)
	{
		for (y=0; y<512; y++)
		{
			t[x*16 + y*512*16 + 0] = (float)x/512.0;
			t[x*16 + y*512*16 + 1] = (float)y/512.0;
			t[x*16 + y*512*16 + 2] = 0.5;
		}
	}
	
	fieldDataUnlock(f);
	
	fieldSend(f, 0, 0, 9);
	fieldSend(f, 1, 1, 9);
	fieldSend(f, 2, 2, 9);
	
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


typedef short float16;

int main(int argc, char *argv[])
{
	x_init();			//Setup exception handling / memory management.
	
	x_try
		mpInit(3);		//Start up enough threads for system
		
		printf("\n\nFluid Server Launching\n");
		pthread_mutex_init(&m, NULL);
		
		error *err = NULL;
		netServer *server = netServerCreate("2048", NETS_TCP, NULL, onConnect);
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
	x_catch(e)
		printf("In Handler: %s\n", errorMsg(e));
	x_finally
	
	mpTerminate();
}
