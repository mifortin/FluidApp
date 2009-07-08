/*
 *  Test_Locality.c
 *  FluidApp
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include "memory.h"

//This file will test the difference in performance that arises due to locality
//of accesses.
//
//For demonstration purposes, a simple Gauss-Siedel matrix solver is used.
//
//For completeness, very bad locality is demonstrated...

double localityTimeFunc()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	
	return (double)t.tv_sec + ((double)t.tv_usec) / 1000000.0;
}

void localityDataInit(float *in_data)
{
	int x;
	for (x=0; x<4096*4096; x++)
		in_data[x] = (float)rand() / (float)RAND_MAX;
}


void localityHybridHitTest2(float *in_data)
{
	int x,y;
	int itr;
	int cy;
	int i2;
	for (i2=0; i2<8; i2++)
	{
		for (cy=1; cy<=5; cy++)
		{
			for (y=cy; y>0; y--)
			{
				for (x=1; x<4095; x++)
				{
					in_data[x+y*4096] =
						(in_data[x+1+y*4096] - in_data[x-1+y*4096] +
						in_data[x+(y+1)*4096] - in_data[x+(y-1)*4096]);
				}
			}
		}
		
		for (cy=6; cy<4095; cy++)
		{
			for (itr=0; itr<5; itr++)
			{
				y = cy-itr;
				for (x=1; x<4095; x++)
				{
					in_data[x+y*4096] =
						(in_data[x+1+y*4096] - in_data[x-1+y*4096] +
						in_data[x+(y+1)*4096] - in_data[x+(y-1)*4096]);
				}
			}
		}
		
		for (cy=1; cy<5; cy++)
		{
			for (itr=cy; itr<5; itr++)
			{
				y =4095- cy-itr;
				for (x=1; x<4095; x++)
				{
					in_data[x+y*4096] =
						(in_data[x+1+y*4096] - in_data[x-1+y*4096] +
						in_data[x+(y+1)*4096] - in_data[x+(y-1)*4096]);
				}
			}
		}
	}
}


void localityHybridHitTest(float *in_data)
{
	int x,y;
	int itr;
	int cy;
	int i2;
	for (i2=0; i2<4; i2++)
	{
		for (cy=1; cy<=10; cy++)
		{
			for (y=cy; y>0; y--)
			{
				for (x=1; x<4095; x++)
				{
					in_data[x+y*4096] =
						(in_data[x+1+y*4096] - in_data[x-1+y*4096] +
						in_data[x+(y+1)*4096] - in_data[x+(y-1)*4096]);
				}
			}
		}
		
		for (cy=11; cy<4095; cy++)
		{
			for (itr=0; itr<10; itr++)
			{
				y = cy-itr;
				for (x=1; x<4095; x++)
				{
					in_data[x+y*4096] =
						(in_data[x+1+y*4096] - in_data[x-1+y*4096] +
						in_data[x+(y+1)*4096] - in_data[x+(y-1)*4096]);
				}
			}
		}
		
		for (cy=1; cy<10; cy++)
		{
			for (itr=cy; itr<10; itr++)
			{
				y =4095- cy-itr;
				for (x=1; x<4095; x++)
				{
					in_data[x+y*4096] =
						(in_data[x+1+y*4096] - in_data[x-1+y*4096] +
						in_data[x+(y+1)*4096] - in_data[x+(y-1)*4096]);
				}
			}
		}
	}
}


void localitySimpleHitTest(float *in_data)
{
	int x,y;
	int itr;
	int cy;
	for (cy=1; cy<=40; cy++)
	{
		for (y=cy; y>0; y--)
		{
			for (x=1; x<4095; x++)
			{
				in_data[x+y*4096] =
					(in_data[x+1+y*4096] - in_data[x-1+y*4096] +
					in_data[x+(y+1)*4096] - in_data[x+(y-1)*4096]);
			}
		}
	}
	
	for (cy=41; cy<4095; cy++)
	{
		for (itr=0; itr<40; itr++)
		{
			y = cy-itr;
			for (x=1; x<4095; x++)
			{
				in_data[x+y*4096] =
					(in_data[x+1+y*4096] - in_data[x-1+y*4096] +
					in_data[x+(y+1)*4096] - in_data[x+(y-1)*4096]);
			}
		}
	}
	
	for (cy=1; cy<40; cy++)
	{
		for (itr=cy; itr<40; itr++)
		{
			y = 4095-cy-itr;
			for (x=1; x<4095; x++)
			{
				in_data[x+y*4096] =
					(in_data[x+1+y*4096] - in_data[x-1+y*4096] +
					in_data[x+(y+1)*4096] - in_data[x+(y-1)*4096]);
			}
		}
	}
}


void localityInOrderTest(float *in_data)
{
	int x,y;
	int itr;
	for (itr=0; itr<800; itr++)
	{
		for (y=1; y<4095; y++)
		{
			for (x=1; x<4095; x++)
			{
				in_data[x+y*4096] =
					(in_data[x+1+y*4096] - in_data[x-1+y*4096] +
					in_data[x+(y+1)*4096] - in_data[x+(y-1)*4096]);
			}
		}
	}
}


void localityBadTest(float *in_data)
{
	int x,y;
	int itr;
	for (itr=0; itr<40; itr++)
	{
		for (x=1; x<4095; x++)
		{
			for (y=1; y<4095; y++)
			{
				in_data[x+y*4096] =
					(in_data[x+1+y*4096] - in_data[x-1+y*4096] +
					in_data[x+(y+1)*4096] - in_data[x+(y-1)*4096]);
			}
		}
	}
}

#include "mpx.h"

void localityTestCoherence(float *in_data, mpCoherence *o)
{
	x_try
	int x;
	
	int tid, fn, data;
	mpCTaskObtain(o, &tid, &fn, &data);
	
	while (tid != -1)
	{
		int y = data+1;
		//printf("Doing: %i %i\n", tid, data);
		for (x=1; x<4095; x++)
		{
			in_data[x+y*4096] =
				(in_data[x+1+y*4096] - in_data[x-1+y*4096] +
				in_data[x+(y+1)*4096] - in_data[x+(y-1)*4096]);
		}
		mpCTaskComplete(o, tid, fn, data, &tid, &fn, &data);
	}
	x_catch(e)
		printf("ERROR: %s\n", errorMsg(e));
	x_finally
}


float *g_data;
mpQueue *q;
atomic_int32	i = 0;

void localityNCoherence(void *in_o)
{
	mpCoherence *o = (mpCoherence*)in_o;
	float *in_data = g_data;
	//int proc = AtomicAdd32Barrier(i, 1);
	x_try
	int x;
	
	
	int tid, fn, data;
	//printf("On proc %i\n", proc);
	mpCTaskObtain(o, &tid, &fn, &data);
	
	while (tid != -1)
	{
		int y = data+1;
		//printf("%i", proc);
		//printf("Doing on %i: %i %i\n", proc, tid, data);
		for (x=1; x<4095; x++)
		{
			in_data[x+y*4096] =
				(in_data[x+1+y*4096] - in_data[x-1+y*4096] +
				in_data[x+(y+1)*4096] - in_data[x+(y-1)*4096]);
		}
		mpCTaskComplete(o, tid, fn, data, &tid, &fn, &data);
	}
	x_catch(e)
		printf("ERROR: %s\n", errorMsg(e));
	x_finally
	
	//printf("Done proc %i\n", proc);
	mpQueuePush(q, NULL);
}


void testLocality()
{
	//L1 cache is 32k on Intel (we'll use this as a base).  
	//L2 cache is 6MB on Intel (this will take effort to bring down).
	//
	//	Therefore, 4096x4096 seems like a good grid size for demonstration
	//	purposes.  This is 64MB, too big to fit into any cache.

	q = mpQueueCreate(4);

	printf(" Benching Locality:\n");
	
	float *data = malloc(sizeof(float) * 4096 * 4096);
	printf("  Alignment: %i\n", (int)(*data)%128 == 0);
	
	localityDataInit(data);
	
	double start = localityTimeFunc();
	/*localityBadTest(data);
	printf("  Out Of Order: %f\n", localityTimeFunc() - start);
	*/
//	localityDataInit(data);
//	start = localityTimeFunc();
//	localityInOrderTest(data);
//	printf("  In Order: %f\n", localityTimeFunc() - start);
	/*
	localityDataInit(data);
	start = localityTimeFunc();
	localitySimpleHitTest(data);
	printf("  Simple Hit: %f\n", localityTimeFunc() - start);
	
	localityDataInit(data);
	start = localityTimeFunc();
	localityHybridHitTest(data);
	printf("  Hybrid Hit: %f\n", localityTimeFunc() - start);
	
	localityDataInit(data);
	start = localityTimeFunc();
	localityHybridHitTest2(data);
	printf("  Hybrid Hit2: %f\n", localityTimeFunc() - start);*/
	
	int z = 128;
	x_init();
	/*for (z=1; z<=1024; z+=z)
	{
		mpCoherence *o = mpCCreate(4094, 800, z);
		
		int x;
		for (x=0; x<800; x++)
			mpCTaskAdd(o, 0, 0, 1, 1);
		
		localityDataInit(data);
		start = localityTimeFunc();
		localityTestCoherence(data, o);
		printf("  Coherence Engine (uniproc cache=%i) :%f\n", 
					z, localityTimeFunc() - start);
		
		x_free(o);
	}/**/
	
	
	mpInit(8);
	for (z=1; z<=1024; z+=z)
	{
		mpCoherence *o = mpCCreate(4094, 800, z);
		
		int x;
		for (x=0; x<800; x++)
			mpCTaskAdd(o, 0, 0, 1, 1);
		
		localityDataInit(data);
		start = localityTimeFunc();
		g_data = data;
		
		mpTaskFlood(localityNCoherence, o);
		
		mpQueuePop(q);
		mpQueuePop(q);
		mpQueuePop(q);
		mpQueuePop(q);
		mpQueuePop(q);
		mpQueuePop(q);
		mpQueuePop(q);
		mpQueuePop(q);
		
		printf("  Coherence Engine (2-proc cache=%i) :%f\n", 
					z, localityTimeFunc() - start);
		
		x_free(o);
	}
	mpTerminate();
	
	free(data);
}
