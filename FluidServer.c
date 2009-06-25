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


int onConnect(void *d, netServer *in_vr, netClient *in_remote)
{
	netInStream *s = netInStreamCreate(in_remote, 256);
	
	int x,ds;
	for (x=0; x<10000000; x++)
	{
		void *dat;
		while ((dat = netInStreamRead(s, &ds)) == NULL) {}
		
		printf("%i: '%s'\n", x, (char*)dat);
		
		netInStreamDoneRead(s);
	}
	
	return 0;
}


int main(int argc, char *argv[])
{
	x_init();			//Setup exception handling / memory management.
	
	x_try
		mpInit(3);		//Start up enough threads for system
		
		printf("\n\nFluid Server Launching\n");
		
		netServer *server = netServerCreate("2048", NETS_TCP, NULL, onConnect);
		printf("Server Launched\n");
		
		netClient *c = netClientCreate("localhost", "2048", NETS_TCP);
		netOutStream *s = netOutStreamCreate(c, 256);
		
		int x;
		for (x=0; x<10000000; x++)
		{
			void *d = netOutStreamBuffer(s, 19);
			sprintf(d, "Testing %i",x);
			netOutStreamSend(s);
		}
		
		x_free(server);		//This blocks the current thread waiting for other
							//threads!
		
		fflush(stdout);
		return 0;
	x_catch(e)
		printf("In Handler: %s\n", errorMsg(e));
	x_finally
	
	mpTerminate();
}
