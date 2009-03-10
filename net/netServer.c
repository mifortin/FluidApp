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


void *netServerConnection(void *eData)
{
	netServer *sData = (netServer*)eData;
	
	if (pthread_mutex_lock(&sData->m_mutex) != 0)	goto done;
	
	netClient *cur = sData->m_client;
	sData->m_client = NULL;
	
	if (pthread_mutex_unlock(&sData->m_mutex) != 0)	goto done;
	
	sData->m_userFunction(sData->m_userData, sData, cur);
	
done:

	netClientFree(cur);

	pthread_mutex_lock(&sData->m_mutex);
	sData->m_runningThreads--;
	pthread_mutex_unlock(&sData->m_mutex);
	
	return NULL;
}


void *netServerThread(void *eData)
{
	netServer *sData = (netServer*)eData;
	
	fd_set selectSet;
	fd_set copySet;
	
	pthread_mutex_t mtx = sData->m_mutex;
	
	if (pthread_mutex_lock(&mtx) != 0)	goto done;
	
	FD_ZERO(&selectSet);
	FD_SET(sData->m_socket, &selectSet);
	
	if (pthread_mutex_unlock(&mtx) != 0)	goto done;
	
	for (;;)
	{
		FD_COPY(&selectSet, &copySet);
		
		//Wait for a bit of time, locking the mutex...
		if (pthread_mutex_lock(&mtx) != 0)	goto done;
		
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
					int clientSock = accept(sData->m_socket,
											&remoteAddress,
											&remoteSize);
					if (clientSock != -1)
					{
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
		
		if (pthread_mutex_unlock(&mtx) != 0)	goto done;
	}

done:
	return NULL;
}


netServer *netServerCreate(char *port, int flags, void *in_d,
									netServerFn_onConnect fn_oConn)
{
	if (!(flags&NETS_UDP) && !(flags&NETS_TCP))
		return NULL;
	
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
		freeaddrinfo(servinfo);
		return NULL;
	}
	
	int one = 1;
	if (setsockopt(mySocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int)) == -1)
	{
		close(socket);
		freeaddrinfo(servinfo);
		return NULL;
	}
	
	if (bind(mySocket, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
	{
		close(socket);
		freeaddrinfo(servinfo);
		return NULL;
	}
	
	freeaddrinfo(servinfo);
	
	if (listen(mySocket, 5) == -1)
	{
		close(socket);
		printf("Failed listening on port\n");
		return NULL;
	}

	netServer *toRet = malloc(sizeof(netServer));
	
	if (toRet)
	{
		memset(toRet, 0, sizeof(netServer));
		
		toRet->m_runningThreads = 0;
		toRet->m_flags = flags;
		toRet->m_socket = mySocket;
		toRet->m_userData = in_d;
		toRet->m_userFunction = fn_oConn;
		
		if (pthread_mutex_init(&toRet->m_mutex, NULL) != 0)
		{
			netServerFree(toRet);
			return NULL;
		}
		
		if (pthread_create(&toRet->m_serverThread, NULL, netServerThread, toRet) != 0)
		{
			pthread_mutex_destroy(&toRet->m_mutex);
			netServerFree(toRet);
			return NULL;
		}
	}
	
	return toRet;
}


void netServerFree(netServer *in_svr)
{
	pthread_mutex_t mtx = in_svr->m_mutex;
	
	pthread_mutex_lock(&mtx);
	
	if (in_svr->m_socket != -1)
		close(in_svr->m_socket);
	in_svr->m_socket = -1;
	
	pthread_mutex_unlock(&mtx);
	pthread_t tmp = in_svr->m_serverThread;
	pthread_join(tmp, NULL);
	pthread_mutex_lock(&mtx);
	
	while (in_svr->m_runningThreads != 0)
	{
		pthread_mutex_unlock(&mtx);
		pthread_mutex_lock(&mtx);
	}
	pthread_mutex_unlock(&mtx);

	memset(in_svr, 0, sizeof(netServer));

	free(in_svr);
	pthread_mutex_destroy(&mtx);
}
