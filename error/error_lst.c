/*
 *  error_lst.c
 *  FluidApp
 *
 */

#include "error_pvt.h"
#include "memory.h"
#include <stdio.h>

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
		
		if (errorList_first == NULL)
		{
			errorList_first = errorList_last = in_error;
		}
		else
		{
			errorList_last->m_list = in_error;
			errorList_last = in_error;
		}
	}
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
