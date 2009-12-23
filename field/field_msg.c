/*
 *  field_msg.c
 *  FluidApp
 */

#include "field.h"
#include "field_pvt.h"
#include "memory.h"
#include <stdlib.h>
#include <string.h>

void fieldMsgFree(void *o)
{ /* NADA */ }


fieldMsg *fieldMsgCreate()
{
	fieldMsg *o = x_malloc(sizeof(fieldMsg), fieldMsgFree);
	memset(o, 0,  sizeof(fieldMsg));
	return o;
}

void fieldMsgReceive(fieldMsg *in_fm, netClient *in_client);
int fieldMsgCount(fieldMsg *in_fm);
int isFieldCharPtr(fieldMsg *in_fm, int in_fld);
int isFieldInt(fieldMsg *in_fm, int in_fld);
const char *fieldCharPtr(fieldMsg *in_fm);
int fieldInt(fieldMsg *in_fm);

void fieldMsgClear(fieldMsg *in_fm)
{
	memset(in_fm, 0, sizeof(fieldMsg));
}

void fieldMsgAddInt(fieldMsg *in_fm, int in_data)
{
}

void fieldMsgAddChar(fieldMsg *in_fm, const char *in_ch);
void fieldMsgSend(fieldMsg *in_fm, netClient *in_client);
