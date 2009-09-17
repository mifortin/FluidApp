/*
 *  fluid_macros_2.h
 *  FluidApp
 */

////////////////////////////////////////////////////////////////////////////////
//
//		A set of macros that are cleaner and more modular
//			(whatever that means ... essentially fluid_macros was getting
//			messy and unworkable...)
//

#ifndef fluid_macros_2_h
#define fluid_macros_2_h

//Utility to get a pointer offseted by characters...
#define fluidFloatPointer(base, offset) \
						((float*)(((char*)base) + (offset)))

//Easy way to read "Minimum"
#define fluidSmallest(a,b)	((a)<(b)?(a):(b))
#define fluidLargest(a,b)	((a)>(b)?(a):(b))

//Clamping is always good...
#define fluidClamp(a,min,max)	fluidSmallest(max, fluidLargest(a,min))

//Basic interpolation
#define fluidLinearInterpolation(sx,sy,zz,oz,zo,oo)		\
						((1-sy)	* ((1-sx) * zz + sx*oz)		\
						+ sy	* ((1-sx) * zo + sx*oo))

//Basic swap
#define fluidSwap(t,a,b)	{t _pvt_swap = a; a=b; b = _pvt_swap;}
#endif
