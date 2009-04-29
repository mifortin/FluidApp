/*
 *  protocol_tree.c
 *  FluidApp
 *
 *		Defines a bunch of functions to deal with the tree structure of
 *		the list of protocols.
 */

#include "protocol_pvt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "memory.h"

void protocolTreeFree(protocolPvt *in_root)
{
	if (in_root)
	{
		if (in_root->m_left)	protocolTreeFree(in_root->m_left);
		if (in_root->m_right)	protocolTreeFree(in_root->m_right);
		
		x_free(in_root->m_data);
		free(in_root);
	}
}


protocolPvt *protocolFindClosest(protocolPvt *in_root, int in_protoID)
{
	if (in_root == NULL)
		return NULL;
	
	for(;;)
	{
		if (in_root->m_name == in_protoID)
			return in_root;
		else if (in_root->m_name < in_protoID)
		{
			if (in_root->m_left == NULL)
				return in_root;
			else
				in_root = in_root->m_left;
		}
		else
		{
			if (in_root->m_right == NULL)
				return in_root;
			else
				in_root = in_root->m_right;
		}
	}
}


error *protocolAdd(protocol *in_proto, int in_protoID, void *in_pvt,
						protocolHandlerFn in_fn)
{
	if (pthread_mutex_lock(&in_proto->m_mutex) != 0)
		return errorCreate(NULL, error_thread, "Failed locking mutex!");
	
	protocolPvt *toAdd = malloc(sizeof(protocolPvt));
	if (toAdd == NULL)
	{
		pthread_mutex_unlock(&in_proto->m_mutex);
		return errorCreate(NULL, error_memory,
								"Not enough memory to register protocol");
	}
	
	memset(toAdd, 0, sizeof(protocolPvt));
	toAdd->m_name = in_protoID;
	toAdd->m_data = in_pvt;
	toAdd->m_fn = in_fn;
	
	if (in_proto->m_root == NULL)
	{
		in_proto->m_root = toAdd;
		x_retain(in_pvt);
	}
	else
	{
		protocolPvt *closest = protocolFindClosest(in_proto->m_root, in_protoID);
		assert(closest != NULL);
		
		if (closest->m_name == in_protoID)
		{
			free(toAdd);
			pthread_mutex_unlock(&in_proto->m_mutex);
			return errorCreate(NULL, error_duplicate,
									"Protocol already exists!");
		}
		
		if (closest->m_name < in_protoID)
			closest->m_left = toAdd;
		else
			closest->m_right = toAdd;
		
		x_retain(in_pvt);
	}
	
	if (pthread_mutex_unlock(&in_proto->m_mutex) != 0)
		return errorCreate(NULL, error_thread, "Failed unlocking mutex");
	
	return NULL;
}
