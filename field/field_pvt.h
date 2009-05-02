/*
 *  field_pvt.h
 *  FluidApp
 */

#ifndef FIELD_PVT_H
#define FIELD_PVT_H

#include "field.h"
#include <pthread.h>

struct field
{
	//Note - assume that everything is tightly packed.
	//	(we have the assumption that everything's in order)
	int m_width, m_height;
	int m_components;
	
	union {
		float *f;
		int *i;
	} r_data;
	
	protocol *m_proto;		//No retain - proto retains us.
	
	//Data for the previous receive.  This is essentially to maintain
	//state.
	int m_prevX;
	int m_prevY;
	int m_prevC;
	
	//Receive handler and object...
	fieldReceiveHandler m_receiveHandler;
	void *m_receiveObj;
	
	//Data lock...
	pthread_mutex_t r_dataLock;
};

#endif
