/*
 *  field_pvt.c
 *  FluidApp
 */

#include "field_pvt.h"
#include "memory.h"

#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void fieldFree(void *in_o)
{
	field *in_f = (field*)in_o;
	
	pthread_mutex_destroy(&in_f->r_dataLock);
	
	if (in_f->r_data.f)
		free(in_f->r_data.f);
}


error *fieldHandleData(protocol *in_proto,
								void *in_pvt,
								int in_size,
								void *in_data)
{
	field *in_f = (field*)in_pvt;
	short *s_data = (short*)in_data;
	
	//Just started receiving - read in the headers...
	if (in_f->m_prevX == 0 && in_f->m_prevY == 0)
	{
		if (s_data[1] != in_f->m_width ||
			s_data[2] != in_f->m_height ||
			s_data[3] < 0 || s_data[3] >= in_f->m_components)
			return errorCreate(NULL, error_net, "Invalid field headers");
		
		//printf("%i %i %i\n",in_f->m_prevC, s_data[3], in_f->m_components);
		in_f->m_prevC = s_data[3];
		s_data+=4;
		in_size -= sizeof(short)*4;
	}
	
	//Take our matrix and loop over it from wherever we are....
	assert(in_size%sizeof(short) == 0);
	
	in_size/=sizeof(short);
	
	int *dstPtr = in_f->r_data.i + (in_f->m_prevX +  in_f->m_prevY * in_f->m_width)
					* in_f->m_components + in_f->m_prevC;
	
	
	if (pthread_mutex_lock(&in_f->r_dataLock) != 0)
		return errorCreate(NULL, error_thread, "Failed locking field");
	
	while (in_size > 0)
	{
		short *d = (short*)dstPtr;
		
		//Cram into d[0] and d[1].
		d[0] = (s_data[0] & 0x8000)		//Copy the sign
				| ((s_data[0] >> 3) & 0x0FFF);	//First part of exponent.
		d[1] = ((s_data[0] << 13) & 0xE000);
		
		dstPtr += in_f->m_components;
		s_data++;
		
		in_size--;
		
		in_f->m_prevX++;
		if (in_f->m_prevX == in_f->m_width)
		{
			in_f->m_prevX = 0;
			in_f->m_prevY++;
			
			if (in_f->m_prevY == in_f->m_height)
			{
				in_f->m_prevY = 0;
				
				error *e = in_f->m_receiveHandler(in_f, in_f->m_prevC,
												  in_f->m_receiveObj);
				if (e) return e;
			}
		}
	}
	
	if (pthread_mutex_unlock(&in_f->r_dataLock) != 0)
		return errorCreate(NULL, error_thread, "Failed unlocking field");
	
	return NULL;
}


field *fieldCreate(protocol *in_proto,
				   int in_width, int in_height, int in_components, 
				   lua_State *in_lua, mpMutex *in_luaLock,
				   error **out_err)
{
	int numData = in_width * in_height * in_components * sizeof(float);
	
	field *toRet = x_malloc(sizeof(field), fieldFree);
	
	if (toRet == NULL)
	{
		*out_err = errorCreate(NULL, error_memory, "Failed creating field");
		return NULL;
	}
	
	if (pthread_mutex_init(&toRet->r_dataLock, NULL) != 0)
	{
		x_free(toRet);
		*out_err = errorCreate(NULL , error_thread, "Failed creating locks");
		return NULL;
	}
	
	toRet->r_data.f = NULL;
	toRet->r_data.f = malloc(numData);
	if (toRet->r_data.f == NULL)
	{
		x_free(toRet);
		*out_err = errorCreate(NULL, error_memory, "Failed creating field data");
		return NULL;
	}
	
	toRet->m_width = in_width;
	toRet->m_height = in_height;
	toRet->m_components = in_components;
	toRet->m_proto = in_proto;
	
	toRet->m_prevX = 0;
	toRet->m_prevY = 0;
	toRet->m_prevC = 0;
	
	memset(toRet->r_data.i, 0, numData);
	
	*out_err = protocolAdd(in_proto, 'feld', toRet, fieldHandleData);
	if (*out_err)
	{
		x_free(toRet);
		return NULL;
	}
	
	return toRet;
}

int fieldWidth(field *in_f)
{
	return in_f->m_width;
}

int fieldHeight(field *in_f)
{
	return in_f->m_height;
}

int fieldComponents(field *in_f)
{
	return in_f->m_components;
}

float *fieldDataLock(field *in_f, error **out_err)
{
	if (pthread_mutex_lock(&in_f->r_dataLock) != 0)
	{
		*out_err = errorCreate(NULL, error_net, "Failed locking field");
		return NULL;
	}
	
	return in_f->r_data.f;
}

error *fieldDataUnlock(field *in_f)
{
	if (pthread_mutex_unlock(&in_f->r_dataLock) != 0)
		return errorCreate(NULL, error_net, "Failed unlocking field");
	
	return NULL;
}

float *fieldData(field *in_f)
{
	return in_f->r_data.f;
}


error *fieldSend(field *in_f, int in_srcPlane, int in_dstPlane, int in_c)
{
	//Let the compiler handle this...
	const short opt[16] = {
		0xFFFF,		//0000
		0xFFFF,		//0001
		0xFFFF,		//0010
		0xFFFF,		//0011
		0xFFFF,		//0100
		0xFFFF,		//0101
		0xFFFF,		//0110
		0xFFFF,		//0111
		0xFFFF,		//1000
		0xFB00,		//1001
		0xFB00,		//1010
		0xFB00,		//1011
		0xFB00,		//1100
		0xFB00,		//1101
		0xFB00,		//1110
		0xFB00,		//1111
	};
	
	if (in_srcPlane < 0 || in_srcPlane >= in_f->m_components)
		return errorCreate(NULL, error_flags, "This field doesn't have plane %i",
											in_srcPlane);
	
	if (pthread_mutex_lock(&in_f->r_dataLock) != 0)
		return errorCreate(NULL, error_thread, "Failed locking field");
	
	//Network overhead is greater...
	int maxProtoSize;
	short *buffer;
	error *err = protocolLockBuffer(in_f->m_proto, &maxProtoSize, (void**)&buffer);
	if (err)	return err;
	
	buffer[0] = (short)in_c;		//0 later on
	buffer[1] = (short)in_f->m_width;
	buffer[2] = (short)in_f->m_height;
	buffer[3] = (short)in_dstPlane;
	int protoUsed = sizeof(short) * 4;
	maxProtoSize -= sizeof(short) * 4;
	buffer += 4;
	
	
	//Copy the data to our buffer (afterwards let the simulation run free!
	//	Like the WIND!!!)
	int *ptr = in_f->r_data.i + in_srcPlane;
	int x;
	int tot = in_f->m_width * in_f->m_height;
	for (x=0; x<tot; x++)
	{
		maxProtoSize-=sizeof(short);
		if (maxProtoSize < 0)
		{
			err = protocolUnlockAndSendBuffer(in_f->m_proto, 'feld', protoUsed);
			if (err)
				return err;
			
			err = protocolLockBuffer(in_f->m_proto,&maxProtoSize,(void**)&buffer);
			maxProtoSize-=sizeof(short);
			protoUsed = 0;
		}
		
		short *cur = (short*)ptr;
		
		//Now this ought to be interesting...
		//	Needs different implementation on INTEL?
		buffer[0] = (cur[0] & 0x8000) |
					((cur[0] << 3) & 0x7FF8) |
					((cur[1] >> 13) & 0x0007);
		buffer[0] =  opt[(buffer[x] & 0x7B00) >> 10] & buffer[x];
		buffer++;
		
		protoUsed += sizeof(short);
		
		ptr += in_f->m_components;
	}
	
	pthread_mutex_unlock(&in_f->r_dataLock);
	
	return protocolUnlockAndSendBuffer(in_f->m_proto, 'feld', protoUsed);
}



error *fieldSetReceiveHandler(field *in_f, void *in_obj,
							  fieldReceiveHandler in_rh)
{
	if (pthread_mutex_lock(&in_f->r_dataLock) != 0)
		return errorCreate(NULL, error_thread, "Failed locking field");
		
	in_f->m_receiveHandler = in_rh;
	in_f->m_receiveObj = in_obj;
	
	if (pthread_mutex_unlock(&in_f->r_dataLock) != 0)
		return errorCreate(NULL, error_thread, "Failed unlocking field");
	
	return NULL;
}

