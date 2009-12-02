/*
 *  error_lst.c
 *  FluidApp
 *
 */

#include "error_pvt.h"
#include "memory.h"
#include <stdio.h>
#include "mpx.h"

pthread_mutex_t errorList_sync = PTHREAD_MUTEX_INITIALIZER;

error *errorList_first = NULL;
error *errorList_last = NULL;

error *errorList_scan = NULL;

void errorListAdd(error *in_error)
{
	if (in_error)
	{		
		//Log out information on the error:
		printf("Error: %s : %i : %s\n",
			   	errorFile(in_error),
			   	errorLine(in_error),
			   	errorMsg(in_error));
		
		x_retain(in_error);
		
		
		x_pthread_mutex_lock(&errorList_sync);
		
		if (errorList_first == NULL)
		{
			errorList_first = errorList_last = in_error;
		}
		else
		{
			errorList_last->m_list = in_error;
			errorList_last = in_error;
		}
		
		x_pthread_mutex_unlock(&errorList_sync);
	}
	else
		printf("Error: NULL\n");
}

void errorListReset()
{
	errorList_scan = errorList_first;
}

error *errorListNext()
{
	if (errorList_scan == NULL)
		return NULL;
	
	error *r = errorList_scan;
	
	errorList_scan = errorList_scan->m_list;
	
	return r;
}
