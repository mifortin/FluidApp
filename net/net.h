/*
 *  net.h
 *  FluidApp
 */

#ifndef NET_H
#define NET_H

#include "error.h"

#define NETS_UDP	0x00000001
#define NETS_TCP	0x00000002

//Creation of clients (connection to another machine)
typedef struct netClient netClient;

netClient *netClientCreate(const char *address, char *port, int flags,
						   error **out_e);
void netClientFree(netClient *client);

//Sends binary data - (no hints, very simple)
//	returns NULL on success, else an error object
error *netClientSendBinary(netClient *client, const void *base, int cnt);

//cnt is in/out.  in is the max size of buffer, out is the amount of data...
error *netClientReadBinary(netClient *client, void *dest, int *cnt, int timeout);

//We read (and wait) until the entire buffer is filled.  A nice abstraction
//to simplify things.
error *netClientGetBinary(netClient *client, void *dest, int cnt, int timeout);

//Basic functions to set up a server... (we handle callbacks, that's it!)
typedef struct netServer netServer;

//Net does bare minimum...  The rest is for specialized code.
typedef int(*netServerFn_onConnect)(void *d, netServer *in_svr,
										netClient *in_remote);

//Creates a server listening on port, with flags.  With a connection, calls
//fn_oConn on a new thread...	The server is on a new thread...
netServer *netServerCreate(char *port, int flags, void *in_d,
												netServerFn_onConnect fn_oConn,
												error **out_error);

//server free is thread-safe.
void netServerFree(netServer *in_svr);

#endif
