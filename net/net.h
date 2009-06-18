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
//
//	Return 0 if a timeout occured, else non-zero.
//
//	An exception is thrown if a timeout is detected mid-stream.  (eg.
//	a data request for a chunk does not complete)
int netClientGetBinary(netClient *client, void *dest, int cnt, int timeout);


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
//
//	buffSize is the size of the internal buffer.
//
//	This implies that netStream must be in control of sending and receiving
//	data as it will prepend/append a chunk size.
netStream *netStreamCreate(netClient *in_client, int buffSize);

//Is there a chunk of data loaded up and ready to be consumed?
//	NULL if no chunk of data present.
const void *netStreamChunk(netClient *in_client);

//Tell that we're done with the chunk.  Then it gets recycled for another
//network read.
void netStreamDoneChunk(netClient *in_client);

//Request a buffer to write to (for the case of sending...)
void *netStreamBuffer(netClient *in_client);

//Sends data across the network.  n_bytes of data stored in in_dat.
//	(could netStream be responsible for the underlying buffer?)
//	Sends the 'send' buffer...
void netStreamSend(netStream *in_strm, int n_bytes);

#endif
