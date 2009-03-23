/*
 *  error_pvt.c
 *  FluidApp
 */

#include "error_pvt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

struct error errorDefault = {NULL, NULL, error_memory, "Failed creating error"};

error *errorCreate(error *in_prev, int in_code, const char *in_text, ...)
{
	error *toRet = malloc(sizeof(error));
	
	if (toRet)
	{
		memset(toRet, 0, sizeof(error));
		toRet->m_code = in_code;
		toRet->m_next = in_prev;
		
		va_list v;
		va_start(v, in_text);
		vasprintf(&toRet->m_string, in_text, v);
		va_end(v);
	}
	else
		return &errorDefault;
	
	return toRet;
}

error *errorReply(error *in_error, int in_reply_code, const char *in_text, ...)
{
	error *toRet = malloc(sizeof(error));
	
	if (toRet)
	{
		memset(toRet, 0, sizeof(error));
		toRet->m_code = in_reply_code;
		
		if (in_error)
		{
			toRet->m_next = in_error->m_next;
			in_error->m_next = NULL;
		}
		
		va_list v;
		va_start(v, in_text);
		vasprintf(&toRet->m_string, in_text, v);
		va_end(v);
	}
	else
		return &errorDefault;
	
	return toRet;
}

error *errorNext(error *in_error)
{
	if (in_error)
		return in_error->m_next;
	else
		return NULL;
}

error *errorReplyTo(error *in_error)
{
	if (in_error)
		return in_error->m_child;
	else
		return NULL;
}

void errorFree(error *in_error)
{
	if (in_error)
	{
		if (in_error->m_next) errorFree(in_error->m_next);
		if (in_error->m_child) errorFree(in_error->m_child);
		
		if (in_error->m_string)
			free(in_error->m_string);
		
		free(in_error);
	}
}

int errorCode(error *in_error)
{
	if (in_error)
		return in_error->m_code;
	else
		return error_none;
}

const char *errorMsg(error *in_error)
{
	if (in_error)
		return (in_error->m_string)?in_error->m_string:"";
	else
		return "";
}
