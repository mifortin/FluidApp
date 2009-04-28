/*
 *  netServer.c
 *  FluidApp
 *
 */

#include "netServer.h"

#include <stdlib.h>
#include <stdio.h>

#include <string.h>

#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>

#include "memory.h"

void *netServerConnection(void *eData)
{
	netServer *sData = (netServer*)eData;
	
	error *e = mpMutexLock(sData->r_mutex);
	if (e != NULL)	goto done;
	
	netClient *cur = sData->m_client;
	sData->m_client = NULL;
	
	e = mpMutexUnlock(sData->r_mutex);
	if (e != NULL) goto done;
	
	sData->m_userFunction(sData->m_userData, sData, cur);
	
done:

	x_free(cur);

	if (e != NULL)	x_free(e);
	
	e = mpMutexLock(sData->r_mutex);
	if (e != NULL)	x_free(e);
	sData->m_runningThreads--;
	e = mpMutexUnlock(sData->r_mutex);
	if (e != NULL)	x_free(e);
	
	return NULL;
}


void *netServerThread(void *eData)
{
	netServer *sData = (netServer*)eData;
	
	fd_set selectSet;
	fd_set copySet;
	
	mpMutex *mtx = sData->r_mutex;
	error *e = NULL;
	
	if (NULL != (e = mpMutexLock(mtx)))	goto done;
	
	FD_ZERO(&selectSet);
	FD_SET(sData->m_socket, &selectSet);
	
	if (NULL != (e = mpMutexUnlock(mtx)))	goto done;
	
	for (;;)
	{
		FD_COPY(&selectSet, &copySet);
		
		//Wait for a bit of time, locking the mutex...
		if (NULL != (e = mpMutexLock(mtx)))	goto done;
		
		if (sData->m_client == NULL)
		{
			if (sData->m_socket != -1)
			{
				struct timeval to;
				to.tv_sec = 5;
				to.tv_usec = 0;
				int sel = select(sData->m_socket+1, &copySet, NULL, NULL, &to);
				
				if (sel == -1)
					goto done;
				
				if (sel != 0)			//Not timed out
				{
					struct sockaddr remoteAddress;
					socklen_t remoteSize;
					remoteSize = sizeof(remoteAddress);
					int clientSock = accept(sData->m_socket,
											&remoteAddress,
											&remoteSize);
					if (clientSock != -1)
					{
						printf("Server got connection\n");
						netClient *client = netClientFromSocket(clientSock);
						if (client != NULL)
						{
							pthread_t tmp;
							sData->m_client = client;
							sData->m_runningThreads++;
							
							pthread_create(&tmp, NULL, netServerConnection,
											eData);
						}
					}
				}
			}
			else
				goto done;
		}
		
		if (NULL != (e = mpMutexUnlock(mtx)))	goto done;
	}

done:
	if (e != NULL)
	{
		printf("%s",errorMsg(e));
		x_free(e);
	}
	printf("Killed Server!\n");
	return NULL;
}


void netServerFree(void *in_o)
{
	netServer *in_svr = (netServer*)in_o;
	mpMutex *mtx = in_svr->r_mutex;
	
	//pthread_mutex_lock(&mtx);
	//	
	//	if (in_svr->m_socket != -1)
	//		close(in_svr->m_socket);
	//	in_svr->m_socket = -1;
	//	
	//	pthread_mutex_unlock(&mtx);

	if (mtx != NULL)
	{
		pthread_t tmp = in_svr->m_serverThread;
		pthread_join(tmp, NULL);

		error *e = mpMutexLock(mtx);
		if (e != NULL)	x_free(e);
		
		while (in_svr->m_runningThreads != 0)
		{
			e = mpMutexUnlock(mtx);
			if (e != NULL)	x_free(e);
			
			e = mpMutexLock(mtx);
			if (e != NULL)	x_free(e);
		}
		e = mpMutexUnlock(mtx);
		if (e != NULL)	x_free(e);
	}
		
	memset(in_svr, 0, sizeof(netServer));
	
	if (mtx) x_free(mtx);
	free(in_svr);
}


netServer *netServerCreate(char *port, int flags, void *in_d,
									netServerFn_onConnect fn_oConn,
									error **out_error)
{
	if (!(flags&NETS_UDP) && !(flags&NETS_TCP))
	{
		*out_error = errorCreate(NULL, error_flags,
					"Need to specify either TCP or UDP when creating server");
		return NULL;
	}
	
	struct addrinfo hints;
	struct addrinfo *servinfo;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = (flags&NETS_UDP)?SOCK_DGRAM:SOCK_STREAM;
	hints.ai_protocol = (flags&NETS_UDP)?IPPROTO_UDP:IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	
	if (getaddrinfo(NULL, port, &hints, &servinfo) != 0)
		return NULL;
	
	int mySocket = socket(	servinfo->ai_family,
							servinfo->ai_socktype,
							servinfo->ai_protocol);
	if (mySocket == -1)
	{
		*out_error = errorCreate(NULL, error_create,
					"Unable to create socket for server");
		freeaddrinfo(servinfo);
		return NULL;
	}
	
	int one = 1;
	if (setsockopt(mySocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int)) == -1)
	{
		*out_error = errorCreate(NULL, error_create, 
					"Unable to set address reuse on server");
		close(mySocket);
		freeaddrinfo(servinfo);
		return NULL;
	}
	
	if (bind(mySocket, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
	{
		*out_error = errorCreate(NULL, error_create, 
					"Serverfailed binding socket");
		close(mySocket);
		freeaddrinfo(servinfo);
		return NULL;
	}
	
	freeaddrinfo(servinfo);
	
	if (listen(mySocket, 5) == -1)
	{
		*out_error = errorCreate(NULL, error_create,
					"Server failed listening on socket");
		close(mySocket);
		printf("Failed listening on port\n");
		return NULL;
	}

	netServer *toRet = x_malloc(sizeof(netServer), netServerFree);
	
	if (toRet)
	{
		memset(toRet, 0, sizeof(netServer));
		
		toRet->m_runningThreads = 0;
		toRet->m_flags = flags;
		toRet->m_socket = mySocket;
		toRet->m_userData = in_d;
		toRet->m_userFunction = fn_oConn;
		toRet->r_mutex = NULL;
		
		toRet->r_mutex = mpMutexCreate(out_error);
		if (toRet->r_mutex == NULL)
		{
			*out_error = errorReply(*out_error, error_create,
						"Failed creating server mutex");
			x_free(toRet);
			return NULL;
		}
		
		if (pthread_create(&toRet->m_serverThread, NULL, netServerThread, toRet) != 0)
		{
			*out_error = errorCreate(NULL, error_create,
						"Failed creating server thread");
			x_free(toRet);
			return NULL;
		}
	}
	
	return toRet;
}

