/*
 *  mp_taskWorld.c
 *  FluidApp
 */

#include "mpx.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef MP_OPENCL
#include <OpenCL/cl.h>
#endif

#define MESSAGE_QUIT		100
#define MESSAGE_ACTION		200

//This is an array of `communications'.  They are placed within an mpQueue
//when the contents are used, and read from an mpQueue to use.  There is
//one per mpQueue entry.
typedef struct
{
	int message;		//What to do...
	mpTaskFn fn;		//The function...
	void *data;			//The data...
} mpTaskWorldCommunication;

typedef struct mpTaskWorld mpTaskWorld;
struct mpTaskWorld
{
	int m_workers;				//Number of workers that we have...

	mpQueue *r_sendQueue;		//Queue to send data...
	mpQueue *r_receiveQueue;	//Queue to receive data...
	mpTaskWorldCommunication *r_comms;	//Queue communication data
	
	//All the pthreads...
	pthread_t *rr_threads;		//All that threading...
	
#ifdef MP_OPENCL
	cl_context r_context;		//OpenCL context
	cl_device_id r_device;
	
	mpQueue *cl_sendQueue;
	mpQueue *cl_recvQueue;
	
	int cl_workers;				//Number of worker threads for CL
	int cl_spawned;				//Did we spawn the threads?
								// (can be quite heavy when using OpenCL)
	
	mpTaskWorldCommunication *cl_comms;	//Queue communication data
	
	//Threads for OpenCL / GPU (we need MORE)
	pthread_t *cl_threads;
#endif

};

mpTaskWorld *g_mpTaskWorld = NULL;

//Task engine
void *mpTaskEngine(void *in_o)
{
	//Task engine just loops until it receives the desired signal.
	x_try
		while (1)
		{
			mpTaskWorldCommunication *cur = mpQueuePop(in_o);
			switch(cur->message)
			{
				case MESSAGE_QUIT:
					return NULL;
					
				case MESSAGE_ACTION:
					cur->fn(cur->data);
					break;
			}
			
			mpQueuePush(g_mpTaskWorld->r_receiveQueue, cur);
		}
	x_catch(err)
		printf("mpTaskEngine failed: %s\n", errorMsg(err));
	x_finally
	
	return NULL;
}

#ifdef MP_OPENCL
void *mpOpenCLTaskEngine(void *in_o)
{
	//Task engine just loops until it receives the desired signal.
	cl_command_queue commQueue = NULL;
	int err;
	x_try
		commQueue = clCreateCommandQueue(g_mpTaskWorld->r_context, 
								   g_mpTaskWorld->r_device,
								   0, &err);
		errorAssert(err == CL_SUCCESS, error_create, "Failed creating command queue!");
		while (1)
		{
			mpTaskWorldCommunication *cur = mpQueuePop(in_o);
			switch(cur->message)
			{
				case MESSAGE_QUIT:
					return NULL;
					
				case MESSAGE_ACTION:
					cur->fn(cur->data);
					break;
			}
			
			mpQueuePush(g_mpTaskWorld->cl_recvQueue, cur);
		}
	x_catch(err)
		printf("mpTaskEngine failed: %s\n", errorMsg(err));
	x_finally
	
	if (commQueue)
		clReleaseCommandQueue(commQueue);
	
	return NULL;
}

void mpNotifyOpenCL(const char *errinfo,
				   const void *private_info, size_t cb, void *user_data)
{
	printf(" -> OpenCL error: %s\n", errinfo);
}

#endif

void mpFree(void *in_o)
{
	//Only one instance, so this is easy!
	errorAssert(g_mpTaskWorld == in_o, error_specify, "Hmm, there's a bug here");
	
	//Wait for all of the task engines to complete....
	int i;
	for (i=0; i<g_mpTaskWorld->m_workers; i++)
	{
		mpTaskWorldCommunication *cur = mpQueuePop(g_mpTaskWorld->r_receiveQueue);
		cur->message = MESSAGE_QUIT;
		mpQueuePush(g_mpTaskWorld->r_sendQueue, cur);
	}
	
	for (i=0; i<g_mpTaskWorld->m_workers; i++)
	{
		x_pthread_join(g_mpTaskWorld->rr_threads[i]);
	}
	
	free(g_mpTaskWorld->rr_threads);
	x_free(g_mpTaskWorld->r_sendQueue);
	x_free(g_mpTaskWorld->r_receiveQueue);
	free(g_mpTaskWorld->r_comms);
	
#ifdef MP_OPENCL
	if (g_mpTaskWorld->cl_workers > 0)
	{
		for (i=0; i<g_mpTaskWorld->cl_workers; i++)
		{
			mpTaskWorldCommunication *cur = mpQueuePop(g_mpTaskWorld->cl_recvQueue);
			cur->message = MESSAGE_QUIT;
			mpQueuePush(g_mpTaskWorld->cl_sendQueue, cur);
		}
		
		for (i=0; i<g_mpTaskWorld->cl_workers; i++)
		{
			x_pthread_join(g_mpTaskWorld->cl_threads[i]);
		}
		
		free(g_mpTaskWorld->cl_threads);
		x_free(g_mpTaskWorld->cl_sendQueue);
		x_free(g_mpTaskWorld->cl_recvQueue);
		free(g_mpTaskWorld->cl_comms);
		
		clReleaseContext(g_mpTaskWorld->r_context);
	}
#endif
}

void mpInit(int in_workers)
{
	errorAssert(g_mpTaskWorld == NULL, error_create, "mpInit called twice!");
	errorAssert(in_workers > 0, error_flags, "Need at least one worker");

	g_mpTaskWorld = x_malloc(sizeof(mpTaskWorld), mpFree);
	
	g_mpTaskWorld->m_workers = in_workers;
	g_mpTaskWorld->r_sendQueue = mpQueueCreate(in_workers * 4);
	g_mpTaskWorld->r_receiveQueue = mpQueueCreate(in_workers * 4);
	
	g_mpTaskWorld->rr_threads = malloc(sizeof(pthread_t*)*in_workers);
	
	g_mpTaskWorld->r_comms = malloc(sizeof(mpTaskWorldCommunication)*in_workers*4);
	
	int i;
	
	for (i=0; i<in_workers*4; i++)
	{
		mpQueuePush(g_mpTaskWorld->r_receiveQueue, g_mpTaskWorld->r_comms+i);
	}
	
	for (i=0; i<in_workers; i++)
	{
		x_pthread_create(&g_mpTaskWorld->rr_threads[i], NULL, mpTaskEngine,
							g_mpTaskWorld->r_sendQueue);
	}
	
#ifdef MP_OPENCL
	cl_uint numGPU = 0;
	g_mpTaskWorld->cl_workers = 0;
	cl_device_id device_id;
	int err = clGetDeviceIDs(NULL, CL_DEVICE_TYPE_GPU, 1, &device_id, &numGPU);
	
	errorAssert(err == CL_SUCCESS, error_create, "Failed querying OpenCL for GPUs");
	
	if (numGPU > 0)
	{
		//Query the GPU to see what it supports!
		cl_int cu;
		err = clGetDeviceInfo(device_id, CL_DEVICE_MAX_COMPUTE_UNITS,
							  sizeof(cl_int), &cu, NULL);
		errorAssert(err == CL_SUCCESS, error_create,
					"Failed querying OpenCL for number of cores on GPU");
		printf("Found %i compute units on the GPU\n", cu);
		g_mpTaskWorld->cl_workers = cu;
		
		if (cu <= 0)	return;
		
		g_mpTaskWorld->r_device = device_id;
		
		g_mpTaskWorld->r_context = clCreateContext(NULL, 1, &device_id,
												   mpNotifyOpenCL, NULL, &err);
		errorAssert(err == CL_SUCCESS, error_create,
					"Failed creating OpenCL context!");
		
		//Create the objects needed to maintain OpenCL...
		g_mpTaskWorld->cl_sendQueue = mpQueueCreate(cu*4);
		g_mpTaskWorld->cl_recvQueue = mpQueueCreate(cu*4);
		g_mpTaskWorld->cl_threads = malloc(sizeof(pthread_t*)*cu);
		g_mpTaskWorld->cl_comms = malloc(sizeof(mpTaskWorldCommunication)*cu*4);
		
		//Perpare the receive queue...
		for (i=0; i<cu*4; i++)
		{
			mpQueuePush(g_mpTaskWorld->cl_recvQueue, g_mpTaskWorld->cl_comms+i);
		}
		
		//Launch a thread per GPU core... (if they are used, they should
		//not be used with heavy CPU work...)
		for (i=0; i<cu; i++)
		{
			x_pthread_create(&g_mpTaskWorld->cl_threads[i], NULL,
							 mpOpenCLTaskEngine, g_mpTaskWorld->cl_sendQueue);
		}
		
		g_mpTaskWorld->cl_spawned = 1;
	}
#endif
}

void mpTerminate()
{
	x_free(g_mpTaskWorld);
	g_mpTaskWorld = NULL;
}


int mpSupportsGPU()
{
#ifdef MP_OPENCL
	return g_mpTaskWorld->cl_workers;
#else
	return 0;
#endif
}


void mpTaskLaunch(mpTaskFn in_task, void *in_obj, int target)
{
	
	mpTaskWorldCommunication *cur = mpQueuePop(g_mpTaskWorld->r_receiveQueue);
	cur->message = MESSAGE_ACTION;
	cur->fn = in_task;
	cur->data = in_obj;
	
	if (target == MP_TASK_GPU)
	{
		errorAssert(mpSupportsGPU() > 0, error_flags, "Requested GPU when none present");
#ifdef MP_OPENCL
		mpQueuePush(g_mpTaskWorld->cl_sendQueue, cur);
#endif
	}
	else
		mpQueuePush(g_mpTaskWorld->r_sendQueue, cur);
}


int mpTaskFlood(mpTaskFn in_task, void *in_obj, int target)
{
	int i;
	
	int itr;
	if (target == MP_TASK_GPU)
	{
		errorAssert(mpSupportsGPU() > 0, error_flags, "Requested GPU when none present");
#ifdef MP_OPENCL
		itr = g_mpTaskWorld->cl_workers;
#endif
	}
	else
		itr = g_mpTaskWorld->m_workers;
	
	for (i=0; i<itr; i++)
	{
		mpTaskLaunch(in_task, in_obj, target);
	}
	
	return g_mpTaskWorld->m_workers;
}
