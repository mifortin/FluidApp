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
#include <signal.h>

#include "net.h"
#include "fluid.h"
#include "error.h"
#include "memory.h"
#include "field.h"
#include "mpx.h"

fluid *r_fluid;
fluidServer *r_server;
fluidMessenger *r_messenger;

int usage()
{
	printf(	"FluidServer   cpu 4   size 512 512\n"
			"==================================\n"
			"cpu tells number of cores to use\n"
			"    for heavy work.  Use less if\n"
			"    Max is running on same machine\n"
			"size is resolution of simulation.\n"
			"     width must be multiple of 32.\n");
	return 0;
}

int main(int argc, char *argv[])
{
	struct sigaction s;
	memset(&s, 0, sizeof(s));
	
	sigset_t ss;
	
	sigemptyset(&ss);
	sigaddset(&ss, SIGPIPE);
	
	s.sa_mask = ss;//SIGPIPE;
	s.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &s, NULL);
	
	x_init();			//Setup exception handling / memory management.
	
	int procs = 0;
	int w = 720;
	int h = 480;
	
	int x;
	for (x=1; x<argc; x++)
	{
		if (strcmp(argv[x], "cpu") == 0)
		{
			if (x+1 >= argc)
				return usage();
			
			x++;
			procs = atoi(argv[x]);
		}
		else if (strcmp(argv[x], "size") == 0)
		{
			if (x + 2 >= argc)
				return usage();
			
			x++;
			w = atoi(argv[x]);
			
			x++;
			h = atoi(argv[x]);
		}
		else
		{
			return usage();
		}
	}
	
	if (procs <= 0 || w<= 0 || h<= 0 || w%32 != 0)
	{
		return usage();
	}
	
	x_try
		mpInit(procs);		//Start up enough threads for system
		
		printf("\n\nFluid Server Launching\n");
		printf(" - cpu %i size %i %i", procs, w, h);
		r_fluid = fluidCreate(w, h);
		printf(" - Created Fluid\n");
		r_server = fluidServerCreate(r_fluid);
		printf(" - Created networking agent\n");
		r_messenger = fluidMessengerCreate(r_fluid, r_server);
		printf(" - Created messenger\n");
		printf("Fluid Server Launched!\n");
		
		double t1 = x_time();
		double totalTime = 0;
		int frames = 0;
		
		int done = 0;
		while (!done)
		{
			double t2 = x_time();
			fluidServerOnFrame(r_server);
			
			frames ++;
			totalTime += (t2-t1);
			t1 = t2;
			
			if (totalTime > 10.0f)
			{
				printf("FPS: %f (%f ms/frame)\n", (float)frames / totalTime, totalTime/(float)frames);
				
				frames = 0;
				totalTime = 0;
			}
			
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
	
	return 0;
}
