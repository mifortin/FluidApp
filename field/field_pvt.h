/*
 *  field_pvt.h
 *  FluidApp
 */

#ifndef FIELD_PVT_H
#define FIELD_PVT_H

#include "field.h"

struct field
{
	//Note - assume that everything is tightly packed.
	//	(we have the assumption that everything's in order)
	int m_width, m_height;
	int m_components;
	
	float *m_data;
};

#endif
