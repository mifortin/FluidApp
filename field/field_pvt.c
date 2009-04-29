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

void fieldFree(void *in_o)
{
	field *in_f = (field*)in_o;
	if (in_f->r_data.f)
		free(in_f->r_data.f);
}


error *fieldHandleData(protocol *in_proto,
								void *in_pvt,
								int in_size,
								void *in_data)
{
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
	memset(toRet->r_data.i, 0, numData);
	
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
	
	//Network overhead is greater...
	int maxProtoSize;
	short *buffer = malloc(in_f->m_width * in_f->m_height * sizeof(short));
	if (buffer == NULL)
		return errorCreate(NULL, error_memory, "Out of memory creating buffer");
	
	//Copy the data to our buffer (afterwards let the simulation run free!
	//	Like the WIND!!!)
	int *ptr = in_f->r_data.i + in_srcPlane;
	int x;
	int tot = in_f->m_width * in_f->m_height;
	for (x=0; x<tot; x++)
	{
		short *cur = (short*)ptr;
		
		//Now this ought to be interesting...
		//	Needs different implementation on INTEL?
		buffer[x] = (cur[0] & 0x8000) |
					((cur[0] << 3) & 0x7FF8) |
					((cur[1] >> 13) & 0x0007);
		buffer[x] =  opt[(buffer[x] & 0x7B00) >> 10] & buffer[x];
		
		ptr += in_f->m_components;
	}
	
	z_stream strm;
	memset(&strm, 0, sizeof(strm));
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	
	if (deflateInit(&strm, in_c) != Z_OK)
		return errorCreate(NULL, error_create, "Failed launching gzip");
	
	strm.avail_in = in_f->m_width * in_f->m_height * sizeof(short);
	strm.next_in = (void*)buffer;
	
	float totSent = 0;
	
	do {
		void *outBuff;
		error *err = protocolLockBuffer(in_f->m_proto, &maxProtoSize, &outBuff);
		if (err)
		{
			deflateEnd(&strm);
			free(buffer);
			return err;
		}
		
		strm.avail_out = maxProtoSize;
		strm.next_out = outBuff;
		
		deflate(&strm,Z_FINISH);
		
		totSent += maxProtoSize - strm.avail_out;
		err = protocolUnlockAndSendBuffer(in_f->m_proto, 'feld',
										  maxProtoSize - strm.avail_out);
		if (err)
		{
			deflateEnd(&strm);
			free(buffer);
			return err;
		}
		
	} while (strm.avail_out == 0);
	
	printf("Compression Ratio: %f\n",
		   totSent / (float) (in_f->m_width * in_f->m_height * sizeof(short)));
	
	free(buffer);
	deflateEnd(&strm);
	
	return NULL;
}
