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

field *fieldCreate(protocol *in_proto, int in_width, int in_height, int in_components);

int fieldWidth(field *in_f);
int fieldHeight(field *in_f);
int fieldComponents(field *in_f);

//Be careful with thread safety!!!
float *fieldData(field *in_f);

#endif
