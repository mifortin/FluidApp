//
//	Field exports functionality for large matrices.  These matrices are
//		normally quite large, and are limited to powers of two.
//

#ifndef FIELD_H
#define FIELD_H

#include "error.h"
#include "lua.h"
#include "mpx.h"
#include "lua.h"
#include "protocol.h"

//Dealing with a single field...
typedef struct field field;

field *fieldCreate(protocol *in_proto,
				   int in_width, int in_height, int in_components,
				   lua_State *in_lua, mpMutex *in_luaLock,
				   error **out_err);

int fieldWidth(field *in_f);
int fieldHeight(field *in_f);
int fieldComponents(field *in_f);
float *fieldData(field *in_f);

//Sends a field over the network.
//	srcPlane and dstPlane are teh source and destination plane.
//		(the client and server are expected to have different purposes
//		for each plane).
//
//	in_c is the compression level.  The higher, the better the compression.
//		basic compression is gzip, however bz2 may be presented as an option.
//
//	This is a very lossy process.  (which will be rectified in due time...)
error *fieldSend(field *in_f, int in_srcPlane, int in_dstPlane, int in_c);

#endif
