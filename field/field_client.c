/*
 *  field_client.c
 *  FluidApp
 */

#include "field_pvt.h"
#include "memory.h"


void fieldClientOnFree(void *o)
{
	fieldClient *fc = (fieldClient*)o;
	
	
	
	if (fc->client)			x_free(fc->client);
	if (fc->fld_sending)	x_free(fc->fld_sending);
}

//Connect to a given host with port.
fieldClient *fieldClientCreate(int in_width, int in_height, int in_components,
							   char *szHost, int in_port)
{
	fieldClient *fc = x_malloc(sizeof(fieldClient), fieldClientOnFree);
	
	return fc;
}

//Send a field
void fieldClientSend(fieldClient *fc, field *f)
{
}
