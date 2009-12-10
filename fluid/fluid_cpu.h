/*
 *  fluid_cpu.h
 *  FluidApp
 *
 */

#ifndef FLUID_CPU_H
#define FLUID_CPU_H
#include "fluid_pvt.h"

////////////////////////////////////////////////////////////////////////////////
//
//			The actual simulation method prototypes
//

void fluid_advection_stam_velocity(fluid *in_f, int rowID, pvt_fluidMode *mode);

void fluid_advection_mccormack_repos(fluid *in_f, int rowID, pvt_fluidMode *mode);

void fluid_repos(fluid *in_f, int y, pvt_fluidMode *mode);

void fluid_genPressure(fluid *in_f, int y, pvt_fluidMode *mode);
void fluid_genPressure_dens(fluid *in_f, int y, pvt_fluidMode *mode);
void fluid_genPressure_densfix(fluid *in_f, int y, pvt_fluidMode *mode);
void fluid_applyPressure(fluid *in_f, int y, pvt_fluidMode *mode);

void fluid_viscosity(fluid *in_f, int y, pvt_fluidMode *mode);

void fluid_vorticity_apply(fluid *in_f, int y, pvt_fluidMode *mode);
void fluid_vorticity_curl(fluid *in_f, int y, pvt_fluidMode *mode);
void fluid_dampen(fluid *in_f, int y, pvt_fluidMode *mode);

void fluid_video_dens2char(fluid *in_f, int y, pvt_fluidMode *mode);
void fluid_input_vel2float(fluid *in_f, int y, pvt_fluidMode *mode);

void fluid_input_char2dens(fluid *in_f, int y, pvt_fluidMode *mode);
void fluid_input_float2vel(fluid *in_f, int y, pvt_fluidMode *mode);

#endif
