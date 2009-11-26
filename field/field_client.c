/*
 *  field_client.c
 *  FluidApp
 */

#include "field_pvt.h"
#include "memory.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


void *fieldClientThread(void *o)
{
	fieldClient *fc = (fieldClient*)o;
	netClient *c = fc->client;

restart:
	pthread_mutex_lock(&fc->mtx);
	fc->allSent = 0;
	x_pthread_cond_wait(&fc->cnd, &fc->mtx);
	int as  = fc->allSent;
	
	if (as == -1)
	{
		pthread_mutex_unlock(&fc->mtx);
		return NULL;
	}
	
	fc->allSent = 2;
	
	pthread_mutex_unlock(&fc->mtx);
	
	struct fieldServerJitMatrix mtx = {htonl('JMTX'),
										htonl(sizeof(mtx)),
										htonl(fieldComponents(fc->fld_sending)),
										htonl(FIELD_JIT_FLOAT32),
										htonl(2)};
	mtx.dataSize = fieldWidth(fc->fld_sending)*fieldHeight(fc->fld_sending)
						*fieldComponents(fc->fld_sending)*4;
	mtx.time = x_time()*1000;
	
	int x;
	for (x=0; x<32;x++)
		mtx.dim[x] = 0;
	for (x=0; x<32; x++)
		mtx.dimStride[x] = 0;
	
	mtx.dim[0] = fieldWidth(fc->fld_sending);
	mtx.dim[1] = fieldHeight(fc->fld_sending);
	mtx.dimStride[0] = fieldStrideX(fc->fld_sending);
	mtx.dimStride[1] = fieldStrideY(fc->fld_sending);
	
	struct fieldServerJitHeader head =	{htonl('JMTX'),
									htonl(sizeof(mtx))};
	
	//printf("SENDING FIELD\n");
	netClientSendBinary(c, &head, sizeof(head));
	netClientSendBinary(c, &mtx, sizeof(mtx));
	netClientSendBinary(c, fieldData(fc->fld_sending), mtx.dataSize);
	//printf("FIELD SENT!\n");
	
	goto restart;
}


void fieldClientOnFree(void *o)
{
	fieldClient *fc = (fieldClient*)o;
	
	pthread_mutex_lock(&fc->mtx);
	fc->allSent = -1;
	x_pthread_cond_signal(&fc->cnd);
	pthread_mutex_unlock(&fc->mtx);
	x_pthread_join(fc->thr);
	
	if (fc->client)			x_free(fc->client);
	if (fc->fld_sending)	x_free(fc->fld_sending);
	
	pthread_mutex_destroy(&fc->mtx);
	pthread_cond_destroy(&fc->cnd);
}

//Connect to a given host with port.
fieldClient *fieldClientCreate(int in_width, int in_height, int in_components,
							   char *szHost, int in_port)
{
	fieldClient *fc = x_malloc(sizeof(fieldClient), fieldClientOnFree);
	memset(fc, 0, sizeof(fieldClient));
	
	fc->fld_sending = fieldCreate(in_width, in_height, in_components);
	
	fc->allSent = 2;
	
	pthread_mutex_init(&fc->mtx, NULL);
	x_pthread_cond_init(&fc->cnd, NULL);
	
	char szPort[64];
	snprintf(szPort, 64, "%i",in_port);
	fc->client = netClientCreate(szHost, szPort, NETS_TCP);
	
	x_pthread_create(&fc->thr, NULL, fieldClientThread, fc);
	
	return fc;
}

//Send a field
void fieldClientSend(fieldClient *fc, field *f)
{
	pthread_mutex_lock(&fc->mtx);
	if (fc->allSent != 0
		|| fieldWidth(fc->fld_sending) != fieldWidth(f)
		|| fieldHeight(fc->fld_sending) != fieldHeight(f)
		|| fieldComponents(fc->fld_sending) != fieldComponents(f)
		|| fieldStrideX(fc->fld_sending) != fieldStrideX(f)
		|| fieldStrideY(fc->fld_sending) != fieldStrideY(f))
	{
		//printf("FieldClientSend: Field dropped\n");
		pthread_mutex_unlock(&fc->mtx);
		return;
	}
	
	memcpy(fieldData(fc->fld_sending), fieldData(f),
		   fieldWidth(f)*fieldHeight(f)*fieldComponents(f)*4);
	
	fc->allSent = 1;
	x_pthread_cond_signal(&fc->cnd);
	pthread_mutex_unlock(&fc->mtx);
}
