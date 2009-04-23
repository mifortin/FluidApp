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
#include "lua.h"
#include <pthread.h>
#include "mp.h"

////////////////////////////////////////////////////////////////////////////////
//
//	Basic protocol
//

typedef struct protocol protocol;

//Note: protocol reads on a seperate thread.  The data is invalidated
//		upon returning from this function (as it blocks the reading)
typedef error*(*protocolHandlerFn)(protocol *proto,		//For responding...
								   void *in_pvt,		//Specified object
								   int in_size,			//Amount of data (bytes)
								   void *in_data);		//Pointer to the data.

//Max data size is in bytes (should be at least a few k)
protocol *protocolCreate(netClient *in_client, int in_maxDataSize,
							error **out_error);

//Add a handler for some protocol
error *protocolAdd(protocol *in_proto, int in_protoID, void *in_pvt,
					protocolHandlerFn in_fn);

//Get the maximum data size (for transmission)
int protocolMaxSize(protocol *in_p);

//Sends properly formatted data over the net
//(thread-safe)
error *protocolSend(protocol *in_p, int in_protoID, int in_size, const void *in_data);


////////////////////////////////////////////////////////////////////////////////
//
//	Extensions for Lua
//		This is a protocol used to send/receive Lua data over the network.
//		The code can be executed as well.
//
//		This is different than the Lua bridge that allows Lua to send data
//		over the network.  (This keeps both components small and simple,
//		and by combining them we get complex behaviour)
//
//			'luae'	- Send end of lua text (at which point it's executed)
typedef struct protocolLua protocolLua;

//Upon creating the protocol to transmit lua, we require a lock that will be
//used to encapsulate all calls to lua.  This code is thread-safe, is yours?
//	Lua script can be executed on another thread, so beware!
protocolLua *protocolLuaCreate(protocol *in_proto, lua_State *in_lua,
							   mpMutex *in_lock, error **out_error);

//Sends the given script (null-terminated string) to whoever is supposed to
//process it.
error *protocolLuaSend(protocol *in_proto, const char *in_script);


////////////////////////////////////////////////////////////////////////////////
//
//	Lua data transfer utilities
//		For this to work - we need some sort of global data store.
//		Something that Lua can access in sync with the other end.  For
//		ease, this is represented as an array.
//
typedef struct protocolFloat protocolFloat;

//Bind this to a protocol, and optionally a lua state.  The lua state - 
//when bound, will get a table called 'float' where it can do 'float.send'
//and 'float.receive'.  These get immediate values - returning the previous
//value when the network is congested.
protocolFloat *protocolFloatCreate(protocol *in_p,
								   int in_numElements,
								   lua_State *in_lua,
								   mpMutex *in_luaLock,
								   error **out_error);

//These methods are for outside of Lua.  Lua is limited to the same limits
float protocolFloatReceive(protocolFloat *in_f, int in_eleNo,
						   error **out_err);
error *protocolFloatSend(protocolFloat *in_f, int in_eleNo, float in_val);



////////////////////////////////////////////////////////////////////////////////
//
//	String data transfer utilities
//		For sending/receiving strings.  Namely Lua error messages!
//		(it should not die as easily!)
//
typedef struct protocolString protocolString;
typedef error*(*protocolStringHandler)(void *in_o, const char *in_szData);

//String has two parts - the handler, and the lua stuff.
//	Lua allows for sending a string (essentially a logging mechanism),
//	whereas 
protocolString *protocolStringCreate(protocol *in_p,
									 lua_State *in_lua,
									 mpMutex *in_luaLock,
									 void *in_objHandler,
									 protocolStringHandler in_handler,
									 error **out_error);

error *protocolStringSend(protocol *in_p, const char *in_string);
#endif
