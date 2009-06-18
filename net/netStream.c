/*
 *  netStream.c
 *  FluidApp
 */

#include "net.h"
#include "mpx.h"
#include "memory.h"
#include <stdlib.h>
#include <string.h>
#include "error.h"

//A very simple object (our stream!!!!)
struct netStream
{
	netClient *client;
	
	//Buffers wrap around...  This is the current index of the start of the
	//buffer
	int curStartInd;
	int curEndInd;
	
	//Number of readers
	int readers;
	
	//This is the size of the buffer.
	int buffSize;
	
	//The buffer with it's data.
	void *r_buffer;
	
	//Thread & lock, as is required by threading...
	pthread_t r_thread;
	pthread_cond_t r_cond;
	pthread_mutex_t r_mutex;	//To protect the allocation values
};


////////////////////////////////////////////////////////////////////////////////
//
//	Simple Destructor
//
void netStreamOnFree(void *in_v)
{
	netStream *o = (netStream*)in_v;
	
	x_pthread_join(o->r_thread);
	pthread_mutex_destroy(&o->r_mutex);
	
	if (o->r_buffer) free(o->r_buffer);
}


////////////////////////////////////////////////////////////////////////////////
//
//	Simple thread...
//
void *netStreamThread(void *in_v)
{
	netStream *o = (netStream*)in_v;
	x_try
		while (1)
		{
			//First - read in how much data we must consume...
			int sizeOfChunk = 0;
			while (netClientGetBinary(o->client, &sizeOfChunk,
							sizeof(int), 900) == 0)
			{ /* Called every 900 seconds */ }
			
			sizeOfChunk = ntohl(sizeOfChunk);
			
			//Second - allocate a region within the buffer.  At this
			//point we may stall if no-one has finished reading their
			//part.
			x_pthread_mutex_lock(&o->r_mutex);
				
			x_pthread_mutex_unlock(&o->r_mutex);
		
			int dest;
			dest = o->curEndInd;
			
			if (dest < o->buffSize)
			{
				int cnt = o->buffSize - o->curEndInd;
				netClientReadBinary(o->client,
									(char*)o->r_buffer + o->curEndInd,
									&cnt,
									32);
				
				x_pthread_mutex_lock(&o->r_mutex);
					o->curEndInd += cnt;
				x_pthread_mutex_unlock(&o->r_mutex);
			}
		}
	x_catch(e)
	
	x_finally

	return NULL;
}


//netStream is built on top of a newClient.  Note that once a client is
//bound to a netStream - all input from the stream will be read by the
//netStream.  (unexpected behaviour may occur on two readers)
//
//	buffSize is the size of the internal buffer.
netStream *netStreamCreate(netClient *in_client, int buffSize)
{
	netStream *toRet = x_malloc(sizeof(netStream), netStreamOnFree);

	errorAssert(buffSize > 0, error_flags, "Need positive buffer size");
	toRet->r_buffer = malloc(buffSize);
	errorAssert(toRet->r_buffer != NULL, error_memory, "Out of memory!");
	
	toRet->client = in_client;
	
	toRet->curStartInd = 0;
	toRet->curEndInd = 0;
	toRet->readers = 0;
	
	x_pthread_mutex_init(&toRet->r_mutex, NULL);
	x_pthread_create(&toRet->r_thread, NULL, netStreamThread, toRet);
	
	return toRet;
}


//

