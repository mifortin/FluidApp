/*
 *  error_pvt.h
 *  FluidApp
 */

#ifndef ERROR_PVT_H
#define ERROR_PVT_H

#include "error.h"

struct error
{
	error *m_next;
	error *m_child;
	
	int m_code;
	
	char *m_string;
};

#endif
