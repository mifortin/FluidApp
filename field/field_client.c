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
	
reconnect:
	while (fc->client == NULL)
	{
		x_try
		{
			char szPort[64];
			snprintf(szPort, 64, "%i",fc->port);
			fc->client = netClientCreate(fc->szHost, szPort, NETS_TCP);
			pthread_mutex_lock(&fc->mtx);
			fc->allSent = 2;
			pthread_mutex_unlock(&fc->mtx);
		}
		x_catch(e)
		{
			errorListAdd(e);
			fc->client = NULL;
			
			pthread_mutex_lock(&fc->mtx);
			x_pthread_cond_wait(&fc->cnd, &fc->mtx);
			int as  = fc->allSent;
			
			if (as == -1)
			{
				return NULL;
			}
			
			pthread_mutex_unlock(&fc->mtx);
			
		}
		x_finally
		{}
	}
	
	x_try
	{
		netClient *c = fc->client;
		
		//Count the number of times we've sent data past 3, start
		//receiving timing info.
		int count = 0;

	restart:
		pthread_mutex_lock(&fc->mtx);
		fc->allSent = 0;
		x_pthread_cond_wait(&fc->cnd, &fc->mtx);
		int as  = fc->allSent;
		
		if (as == -1)
		{
			pthread_mutex_unlock(&fc->mtx);
			if (fc->client)			x_free(fc->client);
			return NULL;
		}
		
		fc->allSent = 2;
		
		pthread_mutex_unlock(&fc->mtx);
		
		struct fieldServerJitMatrix mtx = {htonl('JMTX'),
											htonl(sizeof(mtx)),
											htonl(fieldComponents(fc->fld_sending)),
											htonl(FIELD_JIT_CHAR),
											htonl(2)};
		int dataSize = fieldWidth(fc->fld_sending)*fieldHeight(fc->fld_sending)
							*fieldComponents(fc->fld_sending);
		mtx.dataSize = htonl(dataSize);
		mtx.time = x_time()*1000;
		
		int x;
		for (x=0; x<32;x++)
			mtx.dim[x] = 0;
		for (x=0; x<32; x++)
			mtx.dimStride[x] = 0;
		
		mtx.dim[0] =htonl(fieldWidth(fc->fld_sending));
		mtx.dim[1] = htonl(fieldHeight(fc->fld_sending));
		mtx.dimStride[0] = htonl(fieldStrideX(fc->fld_sending));
		mtx.dimStride[1] = htonl(fieldStrideY(fc->fld_sending));
		
		struct fieldServerJitHeader head =	{htonl('JMTX'),
										htonl(sizeof(mtx))};
		
		//printf("SENDING FIELD\n");
		netClientSendBinary(c, &head, sizeof(head));
		netClientSendBinary(c, &mtx, sizeof(mtx));
		netClientSendBinary(c, fieldCharData(fc->fld_sending), dataSize);
		//printf("FIELD SENT!\n");
		//
		//	- 
		//	- Crystallography (1950's tech)
		//	- Protonomics
		//
		
		if (count < 3)
			count++;
		else
		{
			struct fieldServerJitLatency latency;
			netClientGetBinary(c, &latency, sizeof(latency), 10);
			
			if (htonl(latency.id) == 'JMLP')
			{
				//printf("LATENCY INFO:\n");
				//printf(" - send: %f\n", latency.client_time);
				//printf(" - receive: %f\n", latency.parsed_header);
				//printf(" - complete: %f\n", latency.parsed_done);
			}
			else
			{
				printf("ERROR: NOT LATENCY!!\n");
			}
		}
		
		goto restart;
	}
	x_catch(e)
	{
		errorListAdd(e);
		
		pthread_mutex_lock(&fc->mtx);
		if (fc->client)			x_free(fc->client);
		fc->client = NULL;
		fc->allSent = 11;
		pthread_mutex_unlock(&fc->mtx);
		
		goto reconnect;
	}
	x_finally
	{}
	return NULL;
}


void fieldClientOnFree(void *o)
{
	fieldClient *fc = (fieldClient*)o;
	
	pthread_mutex_lock(&fc->mtx);
	fc->allSent = -1;
	x_pthread_cond_signal(&fc->cnd);
	pthread_mutex_unlock(&fc->mtx);
	x_pthread_join(fc->thr);
	
	if (fc->fld_sending)	x_free(fc->fld_sending);
	
	pthread_mutex_destroy(&fc->mtx);
	pthread_cond_destroy(&fc->cnd);
}

//Connect to a given host with port.
fieldClient *fieldClientCreate(int in_width, int in_height, int in_components,
							   const char *szHost, int in_port)
{
	errorAssert(strlen(szHost) < 255, error_flags, "Host name too long!");
	
	fieldClient *fc = x_malloc(sizeof(fieldClient), fieldClientOnFree);
	memset(fc, 0, sizeof(fieldClient));
	
	fc->port = in_port;
	strcpy(fc->szHost, szHost);
	
	fc->fld_sending = fieldCreateChar(in_width, in_height, in_components);
	
	fc->allSent = 10;
	
	pthread_mutex_init(&fc->mtx, NULL);
	x_pthread_cond_init(&fc->cnd, NULL);
	
	x_try
	{
		x_pthread_create(&fc->thr, NULL, fieldClientThread, fc);
	}
	x_catch(e)
	{
		errorListAdd(e);
	}
	x_finally
	
	return fc;
}

//Send a field
void fieldClientSend(fieldClient *fc, field *f)
{
	pthread_mutex_lock(&fc->mtx);
	if (fc->allSent >= 10)
	{
		fc->allSent = fc->allSent + 1;
		
		if (fc->allSent == 100)
		{
			fc->allSent = 10;
			x_pthread_cond_signal(&fc->cnd);
		}
		
		pthread_mutex_unlock(&fc->mtx);
		return;
	}
	
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
		   fieldWidth(f)*fieldHeight(f)*fieldComponents(f));
	
	fc->allSent = 1;
	x_pthread_cond_signal(&fc->cnd);
	pthread_mutex_unlock(&fc->mtx);
}
