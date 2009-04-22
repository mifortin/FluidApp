/*
 *  netServer.h
 *  FluidApp
 */

#ifndef NETSERVER_H
#define NETSERVER_H

#include <pthread.h>
#include "netClient.h"
#include "net.h"
#include "mp.h"

struct netServer {
	//The socket that the server listens to....
	int m_socket;
	
	//The creation flags....
	int m_flags;
	
	//The data...
	void *m_userData;
	netServerFn_onConnect m_userFunction;
	
	//The thread, and the mutex...
	mpMutex *r_mutex;
	pthread_t m_serverThread;
	
	//Temp work item...
	netClient *m_client;
	
	//Number of threads currently running
	int m_runningThreads;
};

#endif
