/*
 *  protocol_pvt.c
 *  FluidApp
 */

#include "protocol_pvt.h"

#include <stdio.h>
#include <stdlib.h>

protocol *createProtocol(netClient *in_client, int in_maxDataSize,
							error **out_error)
{
	protocol *toRet = NULL;
	
	if (in_maxDataSize < 1024)
	{
		*out_error = errorCreate(*out_error,error_flags,
								"Internal buffer must be at least 1k in size.");
		return NULL;
	}
	
	if (in_client == NULL)
	{
		*out_error = errorCreate(*out_error, error_flags,
								"Protocol requires a working connection.");
		return NULL;
	}
	
	toRet = malloc(sizeof(protocol));
	if (toRet == NULL)
	{
		*out_error = errorCreate(*out_error, error_memory,
								"Unable to allocate protocol data structures.");
		return NULL;
	}
	
	return toRet;
}


void protocolFree(protocol *in_proto)
{
	if (in_proto)
	{
		free(in_proto);
	}
}
