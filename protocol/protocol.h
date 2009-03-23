/*
 *  protocol.h
 *  FluidApp
 */

//Protocol defines a way to specify data types as data is transferred back
//and forth over the wire.
//
//	This is done by registering various <handlers> for the protocols.  These
//	handlers each provide a means to communicate data with lua.
//
//	There's a seprate "read" thread that is used to acquire data.  The data
//	within the "read" thread is polled and understood.
//
//		Data can be purged if it's not used within a reasonable delay...
//
//		Protocol's have a limit of data they can accept.  It's up to the host
//		and client to negotiate the size.  That is - the sub-protocols must
//		adopt a maximum-sized packet.
//
//
//	Data transfer start with 'command', followed by data size, and then the
//	actual data.  Any data size exceeding what the internal buffer can handle
//	is discarded.

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "net.h"
#include "error.h"

typedef struct protocol protocol;
typedef error*(*protocolHandlerFn)(void *in_pvt, int in_size, void *in_data);

//Max data size is in bytes (should be at least a few k)
protocol *createProtocol(netClient *in_client, int in_maxDataSize, error **out_error);
void protocolFree(protocol *in_proto);

error *protocolAdd(protocol *in_proto, int in_protoID, void *in_pvt,
					protocolHandlerFn in_fn);

#endif
