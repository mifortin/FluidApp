/*
 *  fluid_pvt.c
 *  FluidApp
 */

#include "fluid_pvt.h"

#include <stdio.h>
#include <stdlib.h>

fluid *fluidCreate(lua_State *ls, int in_width, int in_height, error **out_err)
{
	fluid *toRet = malloc(sizeof(fluid));
	
	if (toRet == NULL)
	{
		*out_err = errorCreate(NULL, error_memory,
									"Out of memory creating fluid object");
		return NULL;
	}
	
	error *tmpError = NULL;
	toRet->fluidData[0] = fieldCreate(in_width, in_height, 8, &tmpError);
	if (toRet->fluidData[0] == NULL)
	{
		*out_err = errorReply(tmpError, error_create,
									"Failed creating fluid data");
		free(toRet);
		return NULL;
	}
	
	toRet->fluidData[1] = fieldCreate(in_width, in_height, 8, &tmpError);
	if (toRet->fluidData[1] == NULL)
	{
		*out_err = errorReply(tmpError, error_create,
									"Failed creating fluid data");
		fieldFree(toRet->fluidData[0]);
		free(toRet);
		return NULL;
	}
	
	return toRet;
}


void fluidFree(fluid *in_fluid)
{
	if (in_fluid)
	{
		fieldFree(in_fluid->fluidData[0]);
		fieldFree(in_fluid->fluidData[1]);
		
		free(in_fluid);
	}
}
