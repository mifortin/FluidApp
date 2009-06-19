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
}

