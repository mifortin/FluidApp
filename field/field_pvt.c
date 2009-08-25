/*
 *  field_pvt.c
 *  FluidApp
 */

#include "field_pvt.h"
#include "memory.h"
#include "half.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void fieldFree(void *in_o)
{
	field *in_f = (field*)in_o;
	
	if (in_f->r_data.f)
		free(in_f->r_data.f);
}



field *fieldCreate(int in_width, int in_height, int in_components)
{
	int numData = in_width * in_height * in_components * sizeof(float);
	
	field *toRet = x_malloc(sizeof(field), fieldFree);
	
	toRet->r_data.f = NULL;
	toRet->r_data.f = malloc(numData);
	if (toRet->r_data.f == NULL)
	{
		x_free(toRet);
		errorRaise(error_memory, "Failed creating field data");
	}
	
	toRet->m_width = in_width;
	toRet->m_height = in_height;
	toRet->m_components = in_components;
	
	toRet->m_strideX = in_components * sizeof(float);
	toRet->m_strideY = in_components * sizeof(float)*in_width;
	
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


int fieldStrideX(field *in_f)
{
	return in_f->m_strideX;
}

int fieldStrideY(field *in_f)
{
	return in_f->m_strideY;
}

float *fieldData(field *in_f)
{
	return in_f->r_data.f;
}

