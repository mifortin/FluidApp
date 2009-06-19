/*
 *  netInStream.c
 *  FluidApp
 */

#include "net.h"
#include "memory.h"
#include "mpx.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>


//This structure describes a netInStream.  Very simple - most of the work
//is in syncrhonizing the read thread...
struct netInStream
{
	netClient *r_client;
	
	int nBuffSize;				//Total size of the buffer
	int nBuffStart;				//Current start location in buffer (mtx)
	int nBuffEnd;				//Current end location in buffer (mtx)
	int nBuffWrite;				//Current write location start (mtx)
	
	void *r_buffer;				//Pointer to the buffer data
	
	
	pthread_mutex_t r_mutex;	//Mutex to guarantee ranges of data
	pthread_cond_t r_cond;		//Condition variable when mutex blocks
	pthread_t r_thread;			//Second thread used to read data of net.
};


//Called once the retain count reaches zero
void netInStreamOnFree(void *v)
{
	netInStream *o = (netInStream*)v;
	
	x_pthread_mutex_lock(&o->r_mutex);
		o->nBuffWrite = -1;
	x_pthread_mutex_unlock(&o->r_mutex);
	
	x_pthread_cond_signal(&o->r_cond);
	
	x_pthread_join(o->r_thread);
	
	x_free(o->r_client);
	
	if (o->r_buffer)	free(o->r_buffer);
	
	pthread_mutex_destroy(&o->r_mutex);
	pthread_cond_destroy(&o->r_cond);
}


void *netInStreamReader(void *v)
{
	x_try
		netInStream *o = (netInStream*)v;
		
		//This is the only thread that can modify nBuffEnd - however,
		//nBuffEnd is accessible through multiple readers.
		char *buff = (char*)o->r_buffer;
		for (;;)
		{
			int size;
			netClientGetBinary(o->r_client, &size, sizeof(int), 9000);
			
			size = htonl(size);
			
			//If size + 4 exceeds the amount of space left, then block...
			x_pthread_mutex_lock(&o->r_mutex);
			
				if (o->nBuffWrite == -1)
					return NULL;
			
				if (o->nBuffWrite == o->nBuffEnd)
				{
					//At end of the stream - enough space or wrap?
					if (o->nBuffWrite + size + sizeof(int) > o->nBuffSize)
						o->nBuffWrite = 0;
				}
				
				if (o->nBuffWrite <= o->nBuffStart && o->nBuffStart < o->nBuffEnd)
				{
					while (o->nBuffWrite + size + sizeof(int) > o->nBuffStart)
					{
						//Block this thread until there's space...
						x_pthread_cond_wait(&o->r_cond, &o->r_mutex);
					}
					
					if (o->nBuffStart == o->nBuffSize)
					{
						o->nBuffStart = 0;
						o->nBuffEnd = o->nBuffWrite;
					}
					
					if (o->nBuffWrite == -1)
						return NULL;
				}
			x_pthread_mutex_unlock(&o->r_mutex);
			
			//Good, now write the size and load up the data...
			*((int*)(buff + o->nBuffWrite)) = size;
			
			netClientGetBinary(o->r_client, buff + o->nBuffWrite + sizeof(int),
							   size, 9000);
			
			//Update our write index...
			if (o->nBuffWrite == o->nBuffEnd)
			{
				x_pthread_mutex_lock(&o->r_mutex);
					o->nBuffEnd += size + sizeof(int);
				x_pthread_mutex_unlock(&o->r_mutex);
			}
			
			o->nBuffWrite += size + sizeof(int);
		}
	x_catch(e)
	x_finally
	
	return NULL;
}


netInStream *netInStreamCreate(netClient *in_client, int in_buffSize)
{
	netInStream *o = x_malloc(sizeof(netInStream), netInStreamOnFree);
	
	errorAssert(in_buffSize > 0, error_flags, "Buffer size must be positive!");
	
	o->r_client = in_client;
	x_retain(in_client);
	
	o->r_buffer = malloc(in_buffSize);
	o->nBuffSize = in_buffSize;
	o->nBuffStart = 0;
	o->nBuffEnd = 100;
	o->nBuffWrite = 0;
	
	x_pthread_mutex_init(&o->r_mutex, NULL);
	x_pthread_cond_init(&o->r_cond, NULL);
	x_pthread_create(&o->r_thread, NULL, netInStreamReader, o);
	
	return o;
}


//Pass in a pointer to the currently loaded data, NULL if none available...
void *netInStreamRead(netInStream *in_stream, int *out_datSize)
{
	void *toRet = NULL;
	
	x_pthread_mutex_lock(&in_stream->r_mutex);
		if (in_stream->nBuffStart < in_stream->nBuffEnd)
		{
			*out_datSize = *((int*)((char*)in_stream->r_buffer+in_stream->nBuffStart));
			toRet = (void*)((char*)in_stream->r_buffer+sizeof(int));
		}
	x_pthread_mutex_unlock(&in_stream->r_mutex);
	
	return toRet;
}


//Mark the current read as completed.  This will update the index to the start
//of the `safe to read' list...
void netInStreamDoneRead(netInStream *in_stream)
{
	x_pthread_mutex_lock(&in_stream->r_mutex);
		if (in_stream->nBuffStart < in_stream->nBuffEnd)
		{
			in_stream->nBuffStart += sizeof(int)
					+ *((int*)((char*)in_stream->r_buffer+in_stream->nBuffStart));
			
			if (in_stream->nBuffStart == in_stream->nBuffEnd &&
				in_stream->nBuffWrite <= in_stream->nBuffStart)
				in_stream->nBuffStart = in_stream->nBuffSize;
		}
		else
		{
			x_pthread_mutex_unlock(&in_stream->r_mutex);
			errorRaise(error_thread, "Called netInStreamDoneRead without any data!");
		}
	x_pthread_mutex_unlock(&in_stream->r_mutex);
	
	x_pthread_cond_signal(&in_stream->r_cond);
}

