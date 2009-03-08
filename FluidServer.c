/*
 *  FluidServer.c
 *  FluidApp
 */

#include <stdio.h>
#include <pthread.h>

#include "net.h"

volatile int tmp = 0;

pthread_mutex_t m;

int onConnect(void *d, netServer *in_vr, netClient *in_remote)
{
	pthread_mutex_lock(&m);
	tmp = 1000;
	pthread_mutex_unlock(&m);
	
	return 0;
}

int main(int argc, char *argv[])
{
	printf("Fluid Server Launching\n");
	
	pthread_mutex_init(&m, NULL);
	
	netServer *server = netServerCreate("1024", NETS_TCP, NULL, onConnect);
	if (server)
	{
		printf("Server Launched\n");
		
		netClient *client = netClientCreate("localhost","1024",NETS_TCP);
		
		netServerFree(server);
		
		if (client)
		{
			printf("Client connected\n");
		
			netClientFree(client);
		}
		
		pthread_mutex_lock(&m);
		printf("Value of tmp: %i\n", tmp);
		pthread_mutex_unlock(&m);
	}
	else
		printf("Failed launching\n");
	
	fflush(stdout);
	return 0;
}
