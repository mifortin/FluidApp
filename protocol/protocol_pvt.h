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
	
	//The client that we read/write to
	netClient *m_client;
	
	//The other thread (for joining)
	int m_bQuit;
	pthread_mutex_t m_mutex;
	pthread_t m_readThread;
};

//Simple ways to deal with the tree...
void protocolTreeFree(protocolPvt *in_root);
protocolPvt *protocolFindClosest(protocolPvt *in_root, int in_protoID);


//Internal data structure to glue Lua and protocol
typedef struct protocolLua protocolLua;

struct protocolLua
{
	lua_State *m_lua;			//Lua state that we are binding...
	pthread_mutex_t m_lock;		//mutex used to protect the lua state...
};

#endif
