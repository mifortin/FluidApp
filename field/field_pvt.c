/*
 *  field_pvt.c
 *  FluidApp
 */

#include "field_pvt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

field *fieldCreate(int in_width, int in_height, int in_components, error **out_err)
{
	int numData = in_width * in_height * in_components * sizeof(float);
	
	field *toRet = malloc(sizeof(field));
	
	if (toRet == NULL)
	{
		*out_err = errorCreate(NULL, error_memory, "Failed creating field");
		return NULL;
	}
	
	toRet->m_data = malloc(numData);
	if (toRet->m_data == NULL)
	{
		free(toRet);
		*out_err = errorCreate(NULL, error_memory, "Failed creating field data");
		return NULL;
	}
	
	toRet->m_width = in_width;
	toRet->m_height = in_height;
	toRet->m_components = in_components;
	memset(toRet->m_data, 0, numData);
	
	return toRet;
}

void fieldFree(field *in_f)
{
	if (in_f)
	{
		if (in_f->m_data)
			free(in_f->m_data);
		
		free(in_f);
	}
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
	return in_f->m_data;
}
