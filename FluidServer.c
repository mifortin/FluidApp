/*
 *  FluidServer.c
 *  FluidApp
 */

#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "net.h"

volatile int tmp = 0;

pthread_mutex_t m;

int onConnect(void *d, netServer *in_vr, netClient *in_remote)
{
	pthread_mutex_lock(&m);
	tmp = 1000;
	pthread_mutex_unlock(&m);
	
	char buffer[256];
	
	int cnt = 256;
	netClientReadBinary(in_remote, buffer, &cnt, 1);
	
	printf("%s", buffer);
	
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
			char toSend[] = "Hello World\n";
			printf("Client connected\n");
		
			netClientSendBinary(client,toSend,strlen(toSend));
		
			netClientFree(client);
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
