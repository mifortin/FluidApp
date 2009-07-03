/*
 *  FluidServer.c
 *  FluidApp
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

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

double dTime()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	
	return (double)t.tv_sec + ((double)t.tv_usec) / 1000000.0;
}

int onConnect(void *d, netServer *in_vr, netClient *in_remote)
{
	netInStream *s = netInStreamCreate(in_remote, 1024*1024*5);
	
	int x,ds;
	double st = dTime();
	for (x=0; x<10000; x++)
	{
		void *dat;
		while ((dat = netInStreamRead(s, &ds)) == NULL) {}
		
		//printf("%i: '%s'\n", x, (char*)dat);
		
		netInStreamDoneRead(s);
	}
	printf("DONE! %f \n\n", dTime() - st);
	
	return 0;
}


int main(int argc, char *argv[])
{
	x_init();			//Setup exception handling / memory management.
	
	x_try
		mpInit(3);		//Start up enough threads for system
		
		printf("\n\nFluid Server Launching\n");
		
		netServer *server = netServerCreate("2045", NETS_TCP, NULL, onConnect);
		printf("Server Launched\n");
		
		netClient *c = netClientCreate("localhost", "2045", NETS_TCP);
		netOutStream *s = netOutStreamCreate(c, 1024*1024*5);
		
		int x;
		for (x=0; x<10000; x++)
		{
			void *d = netOutStreamBuffer(s, 1024*1024*2);
			sprintf(d, "Testing %i",x);
			netOutStreamSend(s);
		}
		printf("SENT!\n\n");
		
		x_free(server);		//This blocks the current thread waiting for other
							//threads!
		
		fflush(stdout);
		return 0;
	x_catch(e)
		printf("In Handler: %s\n", errorMsg(e));
	x_finally
	
	mpTerminate();
}
