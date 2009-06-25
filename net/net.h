/*
 *  net.h
 *  FluidApp
 */

#ifndef NET_H
#define NET_H

#include "error.h"

#define NETS_UDP	0x00000001
#define NETS_TCP	0x00000002

////////////////////////////////////////////////////////////////////////////////
//Creation of clients (connection to another machine)
typedef struct netClient netClient;

netClient *netClientCreate(const char *address, char *port, int flags);

//Sends binary data - (no hints, very simple)
//	Network issues raise exceptions (we assume perfect net conditions)
void netClientSendBinary(netClient *client, const void *base, int cnt);

//cnt is in/out.  in is the max size of buffer, out is the amount of data...
void netClientReadBinary(netClient *client, void *dest, int *cnt, int timeout);

//We read (and wait) until the entire buffer is filled.  A nice abstraction
//to simplify things.
void netClientGetBinary(netClient *client, void *dest, int cnt, int timeout);


////////////////////////////////////////////////////////////////////////////////
//Basic functions to set up a server... (we handle callbacks, that's it!)
typedef struct netServer netServer;

//Net does bare minimum...  The rest is for specialized code.
typedef int(*netServerFn_onConnect)(void *d, netServer *in_svr,
										netClient *in_remote);

//Creates a server listening on port, with flags.  With a connection, calls
//fn_oConn on a new thread...	The server is on a new thread...
netServer *netServerCreate(char *port, int flags, void *in_d,
												netServerFn_onConnect fn_oConn);


////////////////////////////////////////////////////////////////////////////////
//Creates 'stream' out of a client.  Creates a second thread that indefinitely
//waits on data.  Then the client can query what data has been streamed in
//and update when a complete set of data is available.
//
//	netStreams are not supposed to do any blocking.  An operation that would
//	normally result in blocking will throw an exception.  EG. Poll for data,
//	and make sure enough of the data is already loaded up and ready!
typedef struct netStream netStream;

//netStream is built on top of a newClient.  Note that once a client is
//bound to a netStream - all input from the stream will be read by the
//netStream.  (unexpected behaviour may occur on two readers)
netStream *netStreamCreate(netClient *in_client);

//Number of bytes waiting to be consumed
int netStreamWaiting(netStream *in_strm);

//Returns a pointer to 'n' bytes in the stream.  The pointer
//is invalidated on the next call to a netStream function.
//No data is consumed, thus it remains in the buffer until further
//notice.
void *netStreamPeek(netStream *in_strm, int n_bytes);

//Actually reads data from the stream.  The data is marked as 'consumed'
//and will be used eventually.
void *netStreamRead(netStream *in_strm, int n_bytes);

//Tell the system that a pending netStreamRead has completed, and that the
//data can be used again.  (As long as there are pending reads, no writes
//are done).
void netStreamDone(netStream *in_strm);

#endif
