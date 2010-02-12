/*
 *  fluid_messenger.c
 *  FluidApp
 */

#include "fluid.h"

#include "memory.h"

#include "fluid_macros_2.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

struct fluidMessenger
{
	fluid *f;		//Retained fluid object
	
	fieldMsg *m;	//Used during call-backs...
	
	fluidMessengerHandler handler;
	void *handlingObj;
};


void fluidMessengerOnFree(void *o)
{
	fluidMessenger *fm = (fluidMessenger*)o;
	x_free(fm->f);
	x_free(fm->m);
}


fluidMessenger *fluidMessengerCreate(fluid *in_f)
{
	fluidMessenger *r = x_malloc(sizeof(fluidMessenger), fluidMessengerOnFree);
	
	r->f = in_f;
	x_retain(in_f);
	r->m = fieldMsgCreate();
	
	r->handler = NULL;
	r->handlingObj = NULL;
	
	return r;
}


int fluidMessengerParse(fluidMessenger *fm, fieldMsg *msg, int *offset,
						const char *szCode, const char *szPattern, ...)
{
	if (strcmp(szCode, fieldCharPtr(msg, *offset)) != 0)
		return 0;
	*offset = (*offset) +1;
	
	fieldMsgClear(fm->m);
	fieldMsgAddChar(fm->m, szCode);

	va_list p;
	
	va_start(p, szPattern);
	
	while (*szPattern != '\0')
	{
		if (*offset >= fieldMsgCount(msg))
		{
			printf("==> Not enough data for command %s(%s)\n", szCode,szPattern);
			va_end(p);
			return 0;
		}
		
		if (*szPattern == 'i')
		{
			if (isFieldInt(msg, *offset))
			{
				int *a = va_arg(p, int*);
				*a = fieldInt(msg, *offset);
				fieldMsgAddInt(fm->m, *a);
			}
			else
			{
				printf("==> Invalid arguments to command %s(%s)\n", szCode, szPattern);
				va_end(p);
				return 0;
			}
		}
		else if (*szPattern == 'f')
		{
			if (isFieldFloat(msg, *offset))
			{
				float *a = va_arg(p, float*);
				*a = fieldFloat(msg, *offset);
				fieldMsgAddFloat(fm->m, *a);
			}
			else
			{
				printf("==> Invalid arguments to command %s(%s)\n", szCode, szPattern);
				va_end(p);
				return 0;
			}
		}
		else
		{
			printf("==> Invalid arguments to command %s(%s)\n", szCode, szPattern);
			va_end(p);
			return 0;
		}
		
		szPattern++;
		*offset = (*offset) +1;
	}
	
	va_end(p);
	
	if (fm->handlingObj && fm->handler)
		fm->handler(fm->handlingObj, fm->m);
	
	
	return 1;
}


void fluidMessengerAddHandler(fluidMessenger *fm, fluidMessengerHandler h,
								void *ptr)
{
	fm->handler = h;
	fm->handlingObj = ptr;
}


int fluidMessengerHandleMessage(fluidMessenger *fm, fieldMsg *msg)
{
	int curOffset = 0;
	
	while (curOffset < fieldMsgCount(msg))
	{
		if (!isFieldCharPtr(msg, curOffset))
		{
			printf("==> Command started wihtout a string!\n");
			return 0;
		}
		
		float farg, fx,fy;
		int ix, iy;
		
		if (fluidMessengerParse(fm, msg, &curOffset, "viscosity", "f", &farg))
			fluidSetViscosity(fm->f, fluidClamp(farg, 0,10));
		else if (fluidMessengerParse(fm, msg, &curOffset, "vorticity", "f", &farg))
			fluidSetVorticity(fm->f, fluidClamp(farg,0,10));
		else if (fluidMessengerParse(fm, msg, &curOffset, "timestep", "f", &farg))
			fluidSetTimestep(fm->f, fluidClamp(farg,0,1));
		else if (fluidMessengerParse(fm, msg, &curOffset, "density-fade", "f", &farg))
			fluidSetDensityFade(fm->f, fluidClamp(farg,0,1));
		else if (fluidMessengerParse(fm, msg, &curOffset, "velocity-fade", "f", &farg))
			fluidSetVelocityFade(fm->f, fluidClamp(farg,0,1));
		else if (fluidMessengerParse(fm, msg, &curOffset, "gravity-direction", "ff", &fx, &fx))
		{
			float d = sqrtf(fx*fx + fy*fy);
			
			if (d > 0.01f)
				fluidSetGravityVector(fm->f, fx/d, fy/d);
		}
		else if (fluidMessengerParse(fm, msg, &curOffset, "gravity-magnitude", "f", &farg))
			fluidSetGravityMagnitude(fm->f, farg);
		else if (fluidMessengerParse(fm, msg, &curOffset, "temperature-magnitude", "f", &farg))
			fluidSetTemperatureMagnitude(fm->f, farg);
		else if (fluidMessengerParse(fm, msg, &curOffset, "free-surface-simple", ""))
			fluidFreeSurfaceSimple(fm->f);
		else if (fluidMessengerParse(fm, msg, &curOffset, "free-surface-on", ""))
			fluidFreeSurfaceNone(fm->f);
		else if (fluidMessengerParse(fm, msg, &curOffset, "velocity-out-size", "ii", &ix, &iy))
		{
			if (ix > 0 && iy > 0 && ix < 512 && iy < 512)
				fluidVideoVelocityOutSize(fm->f, ix, iy);
		}
		else
		{
			do {
				curOffset++;
			} while (curOffset < fieldMsgCount(msg) && !isFieldCharPtr(msg, curOffset));
		}
	}
	
	return 1;
}
