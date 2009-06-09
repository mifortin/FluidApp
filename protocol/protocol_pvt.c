/*
 *  protocol_pvt.c
 *  FluidApp
 */

#include "protocol_pvt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

#include "memory.h"

void *protocolReadThread(void *threadData)
{
	protocol *p = (protocol*)threadData;
	for (;;)
	{
		if (pthread_mutex_lock(&p->m_mutex) != 0)	goto done;
		int quitFlag = p->m_bQuit;
		if (pthread_mutex_unlock(&p->m_mutex) != 0)	goto done;
		
		if (quitFlag != 0)	return NULL;
		
		//Do usual logic of reading... (save log somewhere?)
		int name;
		int length;
		error *log;
		netClientGetBinary(p->m_client, &name, sizeof(int), 10);
		
		if (pthread_mutex_lock(&p->m_mutex) != 0)	goto done;
		protocolPvt *tmp = protocolFindClosest(p->m_root, name);
		if (pthread_mutex_unlock(&p->m_mutex) != 0)	goto done;
		
		//Note that the data won't change / get removed so we can play as
		//we wish...
		if (tmp == NULL || tmp->m_name != name)
		{
			printf("Unsupported host operation (%i)\n", name);
			return errorCreate(NULL, error_net, "Unsupported host operation");
		}
		
		netClientGetBinary(p->m_client, &length, sizeof(int), 10);
		
		//Prevent buffer errors
		length = ntohl(length);
		if (length < 0 || length > p->m_maxSize)
		{
			printf("ProtocolRead: tmpDebug: Invalid msg to host (%i)\n",length);
			return errorCreate(NULL, error_net, "Host sent invalid request");
		}
		
		//Read the buffer
		if (pthread_mutex_lock(&p->m_bufferMutex) != 0)	goto done;
		netClientGetBinary(p->m_client, p->m_readBuffer, length, 10);
		
		//Invoke the proper function...
		log = tmp->m_fn(p, tmp->m_data, length, p->m_readBuffer);
		if (log != NULL)
		{
			printf("netClient: %s\n", errorMsg(log));
			return log;
		}
		
		if (pthread_mutex_unlock(&p->m_bufferMutex) != 0) goto done;
	}
	
done:
	printf("ProtocolRead: tmpDebug: Failed locking mutex\n");
	return errorCreate(NULL, error_thread, "Failed locking mutex");
}


void protocolFree(void *o)
{
	protocol *in_proto = (protocol*)o;
	int doJoin = 0;
	pthread_mutex_lock(&in_proto->m_mutex);
	if (in_proto->m_bQuit == 0)
	{
		in_proto->m_bQuit = 1;
		doJoin = 1;
	}
	pthread_mutex_unlock(&in_proto->m_mutex);
	
	error *proto = NULL;
	
	if (doJoin == 1)
		pthread_join(in_proto->m_readThread, (void*)&proto);
	
	if (proto)
		x_free(proto);
	
	if (in_proto->m_root)
		protocolTreeFree(in_proto->m_root);
	
	if (in_proto->m_readBuffer)
		free(in_proto->m_readBuffer);
	
	pthread_mutex_destroy(&in_proto->m_mutex);
	pthread_mutex_destroy(&in_proto->m_bufferMutex);
}


protocol *protocolCreate(netClient *in_client, int in_maxDataSize,
							error **out_error)
{
	protocol *toRet = NULL;
	
	//Compute the maximum size negociating with the remote end (use the smallest)
	int header = 'fana';
	netClientSendBinary(in_client, &header, sizeof(int));
	
	//For now assume same host/client byte ordering...
	
	//Send the maxDataSize
	int sendMaxSize = htonl(in_maxDataSize);
	netClientSendBinary(in_client, &sendMaxSize, sizeof(int));
	
	//Read in - let's see what happens...
	int remoteHeader;
	netClientGetBinary(in_client, &remoteHeader, sizeof(int), 10);
	
	if (remoteHeader != header)
	{
		*out_error = errorCreate(NULL, error_net, "Received invalid header");
		return NULL;
	}
	
	int remote_maxDataSize;
	netClientGetBinary(in_client, &remote_maxDataSize, sizeof(int), 10);
	remote_maxDataSize = ntohl(remote_maxDataSize);
	
	
	if (remote_maxDataSize < in_maxDataSize)
		in_maxDataSize = remote_maxDataSize;
	
	
	if (in_maxDataSize < 1024)
	{
		*out_error = errorCreate(NULL,error_flags,
								"Internal buffer must be at least 1k in size.");
		return NULL;
	}
	
	if (in_client == NULL)
	{
		*out_error = errorCreate(NULL, error_flags,
								"Protocol requires a working connection.");
		return NULL;
	}
	
	toRet = x_malloc(sizeof(protocol), protocolFree);
	if (toRet == NULL)
	{
		*out_error = errorCreate(NULL, error_memory,
								"Unable to allocate protocol data structures.");
		return NULL;
	}
	memset(toRet, 0, sizeof(protocol));
	toRet->m_bQuit = 1;			//To not free thread...
	toRet->m_maxSize = in_maxDataSize;
	toRet->m_client = in_client;
	
	if (pthread_mutex_init(&toRet->m_mutex, NULL) != 0)
	{
		*out_error = errorCreate(NULL, error_create, "Failed creating read mutex");
		x_free(toRet);
		return NULL;
	}
	
	if (pthread_mutex_init(&toRet->m_bufferMutex, NULL) != 0)
	{
		*out_error = errorCreate(NULL, error_create, "Failed creating data mutex");
		x_free(toRet);
		
		return NULL;
	}
	
	toRet->m_readBuffer = malloc(in_maxDataSize);
	if (toRet->m_readBuffer == NULL)
	{
		x_free(toRet);
		*out_error = errorCreate(NULL, error_memory,
								 "Unable to allocate read buffer.");
		return NULL;
	}
	
	
	return toRet;
}



error *protocolSetReadyState(protocol *in_p)
{
	in_p->m_bQuit = 0;
	if (pthread_create(&in_p->m_readThread, NULL, protocolReadThread, in_p) != 0)
	{
		in_p->m_bQuit = 1;
		return errorCreate(NULL, error_thread, "Failed creating read thread");
	}
	
	return NULL;
}


int protocolMaxSize(protocol *p)
{
	return p->m_maxSize;
}


error *protocolSend(protocol *in_p, int in_protoID, int in_size,
					const void *in_data)
{
	if (in_size < 0 || in_size > in_p->m_maxSize)
		return errorCreate(NULL, error_flags, "Size must be in range [%i,%i]",
								0, in_p->m_maxSize);
	
	//First send the protocol ID.
	netClientSendBinary(in_p->m_client, &in_protoID, sizeof(int));
	
	//Now the size
	int toSendSize = htonl(in_size);
	netClientSendBinary(in_p->m_client, &toSendSize, sizeof(int));
	
	netClientSendBinary(in_p->m_client, in_data, in_size);
	
	return NULL;
}

error *protocolLockBuffer(protocol *in_p, int *out_buffSize, void **out_buffData)
{
	if (pthread_mutex_lock(&in_p->m_bufferMutex) != 0)
		return errorCreate(NULL, error_thread, "Failed locking buffer access");
	
	*out_buffSize = protocolMaxSize(in_p);
	*out_buffData = in_p->m_readBuffer;
	
	return NULL;
}


error *protocolUnlockAndSendBuffer(protocol *in_p, int in_protoID, int in_size)
{
	if (in_size > protocolMaxSize(in_p))
		return errorCreate(NULL, error_flags, "Buffer overrun in send request");
	
	error *err = protocolSend(in_p, in_protoID, in_size, in_p->m_readBuffer);
	if (err)	return err;
	
	if (pthread_mutex_unlock(&in_p->m_bufferMutex) != 0)
		return errorCreate(NULL, error_thread, "Failed releasing lock on mutex");
	
	return NULL;
}
