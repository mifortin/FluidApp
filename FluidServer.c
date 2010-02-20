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
#include "fluid.h"
#include "error.h"
#include "memory.h"
#include "field.h"
#include "mpx.h"

volatile int tmp = 0;

#define SERVER_PORT		"2045"

fluid *r_fluid;
fluidServer *r_server;
fluidMessenger *r_messenger;

int main(int argc, char *argv[])
{
	x_init();			//Setup exception handling / memory management.
	
	x_try
		mpInit(4);		//Start up enough threads for system
		
		printf("\n\nFluid Server Launching\n");
		r_fluid = fluidCreate(512, 512);
		r_server = fluidServerCreate(r_fluid);
		r_messenger = fluidMessengerCreate(r_fluid, r_server);
		
		int done = 0;
		while (!done)
		{
			fluidServerOnFrame(r_server);
			
			fieldMsg *m;
			while (m = fluidServerNextMessage(r_server))
			{
				if (isFieldCharPtr(m, 0))
				{
					if (strcmp(fieldCharPtr(m, 0), "quit") == 0)
					{
						done = 1;
						break;
					}
				}
				
				int i;
				printf("Execute: ");
				for (i=0; i<fieldMsgCount(m); i++)
				{
					if (isFieldCharPtr(m, i))
						printf("%s ", fieldCharPtr(m, i));
					else if (isFieldInt(m, i))
						printf("%i ", fieldInt(m, i));
					else if (isFieldFloat(m, i))
						printf("%f ", fieldFloat(m, i));
				}
				printf("\n");
				
				fluidMessengerHandleMessage(r_messenger, m);
			}
		}
		
		x_free(r_server);
		x_free(r_messenger);
		x_free(r_fluid);
		
		return 0;
	x_catch(e)
		printf("In Handler: %s\n", errorMsg(e));
	x_finally
	
	mpTerminate();
}
