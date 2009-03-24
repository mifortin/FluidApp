/*
 *  protocol_float.c
 *  FluidApp
 */

#include "protocol_pvt.h"

protocolFloat *protocolFloatCreate(protocol *in_p,
								   int in_numElements,
								   lua_State *in_lua,
								   pthread_mutex_t in_luaLock,
								   error **out_error)
{
	return NULL;
}


void protocolFloatFree(protocolFloat *in_f)
{
	if (in_f)
	{
		free(in_f);
	}
}

//These methods are for outside of Lua.  Lua is limited to the same limits
float protocolFloatReceive(protocolFloat *in_f, int in_eleNo,
						   error **out_err, float in_default)
{
	return in_default;
}

error *protocolFloatSend(protocolFloat *in_f, int in_eleNo, float in_val)
{
	return NULL;
}
