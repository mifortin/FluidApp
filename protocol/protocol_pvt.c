/*
 *  protocol_pvt.c
 *  FluidApp
 */

#include "protocol_pvt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

void *readThread(void *threadData)
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
		log = netClientGetBinary(p->m_client, &name, sizeof(int), 10);
		if (log != NULL)	return log;
		
		if (pthread_mutex_lock(&p->m_mutex) != 0)	goto done;
		protocolPvt *tmp = protocolFindClosest(p->m_root, name);
		if (pthread_mutex_unlock(&p->m_mutex) != 0)	goto done;
		
		//Note that the data won't change / get removed so we can play as
		//we wish...
		if (tmp->m_name != name)
			return errorCreate(NULL, error_net, "Unsupported host operation");
		
		log = netClientGetBinary(p->m_client, &length, sizeof(int), 10);
		if (log != NULL)	return log;
		length = ntohl(length);
		
		//Prevent buffer errors
		if (length < 0 || length > p->m_maxSize)
			return errorCreate(NULL, error_net, "Host sent invalid request");
		
		//Read the buffer
		log = netClientGetBinary(p->m_client, p->m_readBuffer, length, 10);
		if (log != NULL)	return log;
		
		//Invoke the proper function...
		log = tmp->m_fn(p, tmp->m_data, length, p->m_readBuffer);
		if (log != NULL)	return log;
	}
	
done:
	return errorCreate(NULL, error_thread, "Failed locking mutex");
}


protocol *protocolCreate(netClient *in_client, int in_maxDataSize,
							error **out_error)
{
	protocol *toRet = NULL;
	
	//Compute the maximum size negociating with the remote end (use the smallest)
	error *work = NULL;
	int header = 'fana';
	if ((work = netClientSendBinary(in_client, &header, sizeof(int))) != NULL)
	{
		*out_error = errorCreate(work, error_specify, "Procol failed sending header");
		return NULL;
	}
	
	//For now assume same host/client byte ordering...
	
	//Send the maxDataSize
	int sendMaxSize = htonl(in_maxDataSize);
	if ((work = netClientSendBinary(in_client, &sendMaxSize, sizeof(int))) != NULL)
	{
		*out_error = errorCreate(work, error_specify, "Protocol failed sending size");
		return NULL;
	}
	
	//Read in - let's see what happens...
	int remoteHeader;
	if ((work = netClientGetBinary(in_client, &remoteHeader, sizeof(int), 10)) != NULL)
	{
		*out_error = errorCreate(work, error_specify, "Failed getting header");
		return NULL;
	}
	
	if (remoteHeader != header)
	{
		*out_error = errorCreate(NULL, error_net, "Received invalid header");
		return NULL;
	}
	
	int remote_maxDataSize;
	if ((work = netClientGetBinary(in_client, &remote_maxDataSize, sizeof(int), 10))
			!= NULL)
	{
		*out_error = errorCreate(NULL, error_net,
								 "Failed confirming remote packet size");
		return NULL;
	}
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
	
	toRet = malloc(sizeof(protocol));
	if (toRet == NULL)
	{
		*out_error = errorCreate(NULL, error_memory,
								"Unable to allocate protocol data structures.");
		return NULL;
	}
	memset(toRet, 0, sizeof(protocol));
	toRet->m_bQuit = 1;			//To not free thread...
	
	if (pthread_mutex_init(&toRet->m_mutex, NULL) != 0)
	{
		*out_error = errorCreate(NULL, error_create, "Failed creating read mutex");
		free(toRet);
		return NULL;
	}
	
	toRet->m_readBuffer = malloc(in_maxDataSize);
	if (toRet->m_readBuffer == NULL)
	{
		protocolFree(toRet);
		*out_error = errorCreate(NULL, error_memory,
								 "Unable to allocate read buffer.");
		return NULL;
	}
	
	return toRet;
}


void protocolFree(protocol *in_proto)
{
	if (in_proto)
	{
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
			errorFree(proto);
		
		if (in_proto->m_root)
			protocolTreeFree(in_proto->m_root);
		
		if (in_proto->m_readBuffer)
			free(in_proto->m_readBuffer);
		
		free(in_proto);
	}
}


int protocolMaxSize(protocol *p)
{
	return p->m_maxSize;
}


error *protocolSend(protocol *in_p, int in_protoID, int in_size, void *in_data)
{
	if (in_size < 0 || in_size > in_p->m_maxSize)
		return errorCreate(NULL, error_flags, "Size must be in range [%i,%i]",
								0, in_p->m_maxSize);
	
	if (pthread_mutex_lock(&in_p->m_mutex) != 0)
		return errorCreate(NULL, error_thread, "Failed locking mutex");
	
	//First send the protocol ID.
	error *tmp;
	if (tmp = netClientSendBinary(in_p->m_client, &in_protoID, sizeof(int)))
		return errorCreate(tmp, error_specify, "Failed sending protocol ID");
	
	//Now the size
	int toSendSize = htonl(in_size);
	if (tmp = netClientSendBinary(in_p->m_client, &toSendSize, sizeof(int)))
		return errorCreate(tmp, error_specify, "Failed sending data size");
	
	if (tmp = netClientSendBinary(in_p->m_client, in_data, in_size))
		return errorCreate(tmp, error_specify, "Failed sending data");
	
	if (pthread_mutex_unlock(&in_p->m_mutex) != 0)
		return errorCreate(NULL, error_thread, "Failed unlocking mutex");
	
	return NULL;
}
