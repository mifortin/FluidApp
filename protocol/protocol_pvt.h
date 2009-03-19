/*
 *  protocol_pvt.h
 *  FluidApp
 */

#ifndef PROTOCOL_PVT_H
#define PROTOCOL_PVT_H

#include "protocol.h"

typedef struct protocolPvt protocolPvt;

struct protocolPvt
{
	int m_name;					//The name of the protocol
	void *m_data;				//Some data that thte protocol needs to run
	protocolHandlerFn m_fn;		//The function that understands the data

	protocolPvt *m_left;		//The lesser valued names,
	protocolPvt *m_right;
};


struct protocol
{
	//Maximum size of our read buffer
	int m_maxSize;
	
	//Our read buffer!
	void *m_readBuffer;

	//b-tree of all the protocols.
	protocolPvt *m_root;
};


#endif
