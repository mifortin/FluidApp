/*
 *  netClient.c
 *  FluidApp
 */

#include "netClient.h"

#include <stdlib.h>

#include <string.h>

#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>

netClient *netClientFromSocket(int socket)
{
	netClient *toRet = malloc(sizeof(netClient));
	if (toRet)
	{
		memset(toRet, 0, sizeof(netClient));
		toRet->m_socket = socket;
	}
	return toRet;
}

netClient *netClientCreate(char *address, char *port, int flags)
{
	if (!(flags&NETS_UDP) && !(flags&NETS_TCP))
		return NULL;
	
	struct addrinfo hints;
	struct addrinfo *servinfo;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = (flags&NETS_UDP)?SOCK_DGRAM:SOCK_STREAM;
	hints.ai_protocol = (flags&NETS_UDP)?IPPROTO_UDP:IPPROTO_TCP;
	
	if (getaddrinfo(address, port, &hints, &servinfo) != 0)
		return NULL;
	
	int mySocket = socket(	servinfo->ai_family,
							servinfo->ai_socktype,
							servinfo->ai_protocol);
	if (mySocket == -1)
	{
		freeaddrinfo(servinfo);
		return NULL;
	}
	
	if (connect(mySocket, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
	{
		freeaddrinfo(servinfo);
		close(mySocket);
		return NULL;
	}
	
	freeaddrinfo(servinfo);
	
	return netClientFromSocket(mySocket);
}


void netClientFree(netClient *client)
{
	if (client->m_socket)
		close(client->m_socket);
	
	memset(client, 0, sizeof(netClient));
}
