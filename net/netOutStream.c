/*
 *  netOutStream.c
 *  FluidApp
 */

#include "net.h"
#include "memory.h"
#include "mpx.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>

//netOutStream is very similar to a netInStream, but the synchronization
//primitives are slightly different.
//
//	(consumer/sender is in different thread)
struct netOutStream
{
	netClient *r_client;		//Connection to network
	
	int nBuffSize;				//Total size of the buffer
	int nBuffStart;				//Start of the buffer (can loop back to start)
	int nBuffEnd;				//End of buffer (for reading)
	int nBuffWrite;				//End of buffer (for writing)
	
	void *r_buffer;				//Data
	
	pthread_mutex_t r_mutex;	//Mutex to protect the `region' vars
	pthread_cond_t r_cond;		//To unblock stream sending data over network
	pthread_t r_thread;			//Thread sending data over network
};


//Called once the retain count reaches zero
void netOutStreamOnFree(void *v)
{
	netOutStream *o = (netOutStream*)v;
	
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


//Second thread that is practically continually blocked while writing
//stuff.  (If the buffer overflows, the writing stream can block as well)
void *netOutStreamWriter(void *v)
{
	x_try
		netOutStream *o = (netOutStream*)v;
		
		for (;;)
		{
			//Now, first thing is to block if we don't have any data...
			x_pthread_mutex_lock(&o->r_mutex);
				
			x_pthread_mutex_unlock(&o->r_mutex);
			
			//Ship the data across network
			
			//Mark data as shipped and resume!
		}
		
	x_catch(e)
	x_finally
	
	return NULL;
}


//Creates a netOutStream for the given client using the given buffer size.
netOutStream *netOutStreamCreate(netClient *in_client, int in_buffSize)
{
	errorAssert(in_buffSize > sizeof(int), error_flags,
				"Buffer size must be greater than sizeof(int)");
	errorAssert(in_client != NULL, error_flags,
				"Need a valid netClient to bind to.");
	
	netOutStream *o = x_malloc(sizeof(netOutStream), netOutStreamOnFree);
	
	o->r_buffer = malloc(in_buffSize);
	o->nBuffSize = in_buffSize;
	o->nBuffStart = 0;
	o->nBuffEnd = 0;
	o->nBuffWrite = 0;
	
	x_pthread_mutex_init(&o->r_mutex, NULL);
	x_pthread_cond_init(&o->r_cond, NULL);
	x_pthread_create(&o->r_thread, NULL, netOutStreamWriter, o);
	
	return o;
}


//This function will lincrease the buffEnd...
void *netOutStreamBuffer(netOutStream *in_oStream, int in_buffSize)
{
	return NULL;		//STUB
}


//This function just unblocks the writer so that it can do it's work.
void netOutStreamSend(netOutStream *in_oStream)
{
	//STUB
}

