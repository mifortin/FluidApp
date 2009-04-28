//
//	Field exports functionality for large matrices.  These matrices are
//		normally quite large, and are limited to powers of two.
//

#ifndef FIELD_H
#define FIELD_H

#include "error.h"
#include "lua.h"

//Dealing with a single field...
typedef struct field field;

field *fieldCreate(int in_width, int in_height, int in_components, error **out_err);
void fieldFree(field *in_f);

int fieldWidth(field *in_f);
int fieldHeight(field *in_f);
int fieldComponents(field *in_f);
float *fieldData(field *in_f);

//Utility functions to convert from 32-bit float to 16-bit float
#define float32to16(x)	

#endif
