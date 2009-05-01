/*
 *  fluid_cpuCore.c
 *  FluidApp
 *
 */

#include "fluid_cpuCore.h"

fluidStreamDesc FSStreamDescMakeCharacter2D(unsigned char *data, unsigned char*defaults, int strideX, int strideY, int width, int height, int components)
{
	fluidStreamDesc toRet = {0, 0, strideX, strideY, width, height, components, FSCPU_Type_Character2D};
	toRet.data.c = data;
	toRet.defaults.c = defaults;
	return toRet;
}

fluidStreamDesc FSStreamDescMakeFloat2D(float *data,float*defaults, int strideX, int strideY, int width, int height, int components)
{
	fluidStreamDesc toRet = {0, 0,strideX, strideY, width, height, components, FSCPU_Type_Float2D};
	toRet.data.f = data;
	toRet.defaults.f = defaults;
	return toRet;
}
