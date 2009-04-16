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
#include "fluid.h"
#include "protocol.h"

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
	protocol *p = protocolCreate(in_remote, 1024*4, &pError);
	protocolLua *pl = protocolLuaCreate(p, L, m, &pError);
	protocolFloatCreate(p, 10, L, m, &pError);
	
	if (p == NULL)
	{
		printf("Failed creating protocol: %s\n", errorMsg(pError));
		errorFree(pError);
		return 0;
	}
	
	for (;;);
	
	protocolLuaFree(pl);
	protocolFree(p);
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
		
		//NOTE: client might get destroyed before the server in this eg.
		netClient *client = netClientCreate("localhost","2048",NETS_TCP, &err);
		
		if (client)
		{
			error *pError = NULL;
			protocol *p = protocolCreate(client, 1024*4, &pError);
			if (p == NULL)
			{
				printf("Failed creating protocol: %s\n", errorMsg(pError));
				errorFree(pError);
			}
			else
			{
			
				char tmp[256];
				do {
					printf("CLIENT> ");
					scanf("%s", tmp);
					netClientSendBinary(client,tmp,strlen(tmp));
				} while (strcmp(tmp, "quit") != 0);
				
				protocolFree(p);
			
				netClientFree(client);
			}
		}
		else
			printf("Failed client: %s\n", errorMsg(err));
		
		netServerFree(server);
		
		pthread_mutex_lock(&m);
		printf("Value of tmp: %i\n", tmp);
		pthread_mutex_unlock(&m);
	}
	else
		printf("Failed launching: %s\n", errorMsg(err));
	
	fflush(stdout);
	return 0;
}
