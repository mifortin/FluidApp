/*
 *  fluid_pvt.h
 *  FluidApp
 */

#ifndef FLUID_PVT_H
#define FLUID_PVT_H

#include "fluid.h"
#include "field.h"

struct fluid
{
	//	0 - velocity
	//	1 -
	//	2 - pressure
	//	3 - free surfaces
	//	4 - collisions
	//	5 - 
	//	6 - position/repos
	//	7 -
	field *fluidData[2];
	
	int m_curField;
	
	lua_State *m_ls;
	char *m_szGlobName;
};

//functions (utilities)
field *fluid_curField(fluid *in_f);
field *fluid_destField(fluid *in_f);
void fluid_swapField(fluid *in_f);

//functions (public to lua)
void fluid_advection_stam(fluid *in_f);

//Lua interop functions
int fluid_advection_lua(lua_State *in_l);

#endif
