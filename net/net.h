/*
 *  net.h
 *  FluidApp
 */

#ifndef NET_H
#define NET_H

#define NETS_UDP	0x00000001
#define NETS_TCP	0x00000002

//Creation of clients (connection to another machine)
typedef struct netClient netClient;

netClient *netClientCreate(char *address, char *port, int flags);
void netClientFree(netClient *client);

//Basic functions to set up a server... (we handle callbacks, that's it!)
typedef struct netServer netServer;

typedef int(*netServerFn_onConnect)(void *d, netServer *in_svr, netClient *in_remote);

//Creates a server listening on port, with flags.  With a connection, calls
//fn_oConn on a new thread...	The server is on a new thread...
netServer *netServerCreate(char *port, int flags, void *in_d,
												netServerFn_onConnect fn_oConn);

//server free is thread-safe.
void netServerFree(netServer *in_svr);

#endif
