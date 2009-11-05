/*
 *  fluid_pvt.c
 *  FluidApp
 */

#include "fluid_pvt.h"
#include "fluid_macros_2.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "memory.h"


pvt_fluidMode make_advection_stam_velocity(field *srcVelX, field *srcVelY,
											field *dstVelX, field *dstVelY,
										   	float timestep)
{
	pvt_fluidMode toRet;
	
	toRet.advection_stam_velocity.srcVelX = srcVelX;
	toRet.advection_stam_velocity.srcVelY = srcVelY;
	toRet.advection_stam_velocity.dstVelX = dstVelX;
	toRet.advection_stam_velocity.dstVelY = dstVelY;
	toRet.advection_stam_velocity.timestep = timestep;
	
	
	return toRet;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Private data structure and methods to operate on it
//
void fluidFree(void *in_o)
{
	fluid *o = (fluid*)in_o;
	
	if (o->r_coherence)		x_free(o->r_coherence);
	
	if (o->r_blocker)		x_free(o->r_blocker);
	
	if (o->r_pressure)		x_free(o->r_pressure);
	
	if (o->r_density)		x_free(o->r_density);
	if (o->r_density_swap)	x_free(o->r_density_swap);
	
	if (o->r_velocityX)		x_free(o->r_velocityX);
	if (o->r_velocityY)		x_free(o->r_velocityY);
	
	if (o->r_tmpVelX)		x_free(o->r_tmpVelX);
	if (o->r_tmpVelY)		x_free(o->r_tmpVelY);
	
	if (o->r_reposX)		x_free(o->r_reposX);
	if (o->r_reposY)		x_free(o->r_reposY);
}


fluid *fluidCreate(int in_width, int in_height)
{
	fluid *toRet = x_malloc(sizeof(fluid), fluidFree);
	memset(toRet, 0, sizeof(fluid));
	
	toRet->r_velocityX = fieldCreate(in_width, in_height, 1);
	toRet->r_velocityY = fieldCreate(in_width, in_height, 1);
	toRet->r_tmpVelX = fieldCreate(in_width, in_height, 1);
	toRet->r_tmpVelY = fieldCreate(in_width, in_height, 1);
	toRet->r_reposX = fieldCreate(in_width, in_height, 1);
	toRet->r_reposY = fieldCreate(in_width, in_height, 1);
	toRet->r_pressure = fieldCreate(in_width, in_height, 1);
	toRet->r_density = fieldCreate(in_width, in_height, 3);
	toRet->r_density_swap = fieldCreate(in_width, in_height, 3);
	
	toRet->r_blocker = mpQueueCreate(2);
	
	//NOTE: Make this configurable????
	toRet->r_coherence = mpCCreate(in_height, 128, 512);
	
	toRet->m_curField = 0;
	
	toRet->m_usedFunctions = 0;
	
	toRet->m_viscosity = 1.0f;
	toRet->m_vorticity = 1.0f;
	toRet->m_timestep = 1.0f;
	
	toRet->m_fadeVel = 1.0f;
	toRet->m_fadeDens = 1.0f;
	
	toRet->flags = 0;
	
	int i;
	for (i=0; i<TIME_TOTAL; i++)
		toRet->m_times[i] = 0;
	
	return toRet;
}


void fluidSetViscosity(fluid *f, float in_v)
{
	f->m_viscosity = in_v;
}


void fluidSetVorticity(fluid *f, float in_v)
{
	f->m_vorticity = in_v;
}

void fluidSetTimestep(fluid *f, float in_v)
{
	f->m_timestep = in_v;
}

void fluidSetDensityFade(fluid *f, float in_v)
{
	f->m_fadeDens = in_v;
}

void fluidSetVelocityFade(fluid *f, float in_v)
{
	f->m_fadeVel = in_v;
}

void fluidFreeSurfaceNone(fluid *f)
{
	f->flags = (f->flags & (~FLUID_SIMPLEFREE));
}

void fluidFreeSurfaceSimple(fluid *f)
{
	f->flags = (f->flags | FLUID_SIMPLEFREE);
}

void fluidEnableTimers(fluid *f)
{
	f->flags = (f->flags | FLUID_TIMERS);
}

void fluidDisableTimers(fluid *f)
{
	f->flags = (f->flags & (~FLUID_TIMERS));
}


void fluidTaskAddForwardAdvection(fluid *f)
{
	int curFn = f->m_usedFunctions;
	errorAssert(curFn<MAX_FNS, error_memory, "Too many different tasks!");
	
	f->m_fns[curFn].fn = fluid_advection_stam_velocity;
	f->m_fns[curFn].mode = make_advection_stam_velocity(
								f->r_velocityX, f->r_velocityY,
								f->r_tmpVelX,   f->r_tmpVelY,
								f->m_timestep);
	f->m_fns[curFn].times = f->m_times + TIME_ADVECTION;
	
	mpCTaskAdd(f->r_coherence, curFn, -10, 10, 1);
	
	f->m_usedFunctions = curFn+1;
}


void fluidTaskAddBackwardAdvection(fluid *f)
{
	int curFn = f->m_usedFunctions;
	errorAssert(curFn<MAX_FNS, error_memory, "Too many different tasks!");
	
	f->m_fns[curFn].fn = fluid_advection_stam_velocity;
	f->m_fns[curFn].mode = make_advection_stam_velocity(
														f->r_tmpVelX, f->r_tmpVelY,
														f->r_reposX,   f->r_reposY,
														-f->m_timestep);
	f->m_fns[curFn].times = f->m_times + TIME_ADVECTION;
	
	mpCTaskAdd(f->r_coherence, curFn, -10, 10, 1);
	
	f->m_usedFunctions = curFn+1;
}


void fluidTaskCorrectorRepos(fluid *f)
{
	int curFn = f->m_usedFunctions;
	errorAssert(curFn<MAX_FNS, error_memory, "Too many different tasks!");
	
	f->m_fns[curFn].fn = fluid_advection_mccormack_repos;
	f->m_fns[curFn].mode.mccormack_vel_repos.srcVelX = f->r_velocityX;
	f->m_fns[curFn].mode.mccormack_vel_repos.srcVelY = f->r_velocityY;
	f->m_fns[curFn].mode.mccormack_vel_repos.srcAdvX = f->r_tmpVelX;
	f->m_fns[curFn].mode.mccormack_vel_repos.srcAdvY = f->r_tmpVelY;
	f->m_fns[curFn].mode.mccormack_vel_repos.srcErrVelX = f->r_reposX;
	f->m_fns[curFn].mode.mccormack_vel_repos.srcErrVelY = f->r_reposY;
	f->m_fns[curFn].mode.mccormack_vel_repos.dstVelX = f->r_velocityX;
	f->m_fns[curFn].mode.mccormack_vel_repos.dstVelY = f->r_velocityY;
	f->m_fns[curFn].mode.mccormack_vel_repos.dstReposX = f->r_reposX;
	f->m_fns[curFn].mode.mccormack_vel_repos.dstReposY = f->r_reposY;
	f->m_fns[curFn].mode.mccormack_vel_repos.timestep = f->m_timestep;
	f->m_fns[curFn].times = f->m_times + TIME_ADVECTION;
	mpCTaskAdd(f->r_coherence, curFn, -1, 1, 1);
	
	f->m_usedFunctions = curFn+1;
}


void fluidTaskAdvectDensity(fluid *f)
{
	int curFn = f->m_usedFunctions;
	errorAssert(curFn<MAX_FNS, error_memory, "Too many different tasks!");
	
	f->m_fns[curFn].fn = fluid_repos;
	f->m_fns[curFn].mode.repos.reposX = f->r_reposX;
	f->m_fns[curFn].mode.repos.reposY = f->r_reposY;
	f->m_fns[curFn].mode.repos.src = f->r_density;
	f->m_fns[curFn].mode.repos.dst = f->r_density_swap;
	f->m_fns[curFn].times = f->m_times + TIME_ADVECTION;
	
	mpCTaskAdd(f->r_coherence, curFn, -1, 1, 1);
	
	f->m_usedFunctions = curFn+1;
	
	//In the future, this is true!
	fluidSwap(field*, f->r_density, f->r_density_swap);
}


void fluidTaskPressure(fluid *f, int in_iterations, field *in_density)
{
	int curFn = f->m_usedFunctions;
	errorAssert(curFn<MAX_FNS, error_memory, "Too many different tasks!");
	
	if (in_density != NULL)
	{
		f->m_fns[curFn].fn = fluid_genPressure;
		f->m_fns[curFn].mode.pressure.velX = f->r_velocityX;
		f->m_fns[curFn].mode.pressure.velY = f->r_velocityY;
		f->m_fns[curFn].mode.pressure.pressure = f->r_pressure;
		f->m_fns[curFn].mode.pressure.density = in_density;
		f->m_fns[curFn].fn = fluid_genPressure_dens;
		f->m_fns[curFn].times = f->m_times + TIME_PRESSURE;
		
		mpCTaskAdd(f->r_coherence, curFn, -1, 1, 1);
		
		f->m_usedFunctions = curFn+1;
		curFn = f->m_usedFunctions;
		errorAssert(curFn<MAX_FNS, error_memory, "Too many different tasks!");
	}
		
	f->m_fns[curFn].fn = fluid_genPressure;
	f->m_fns[curFn].mode.pressure.velX = f->r_velocityX;
	f->m_fns[curFn].mode.pressure.velY = f->r_velocityY;
	f->m_fns[curFn].mode.pressure.pressure = f->r_pressure;
	f->m_fns[curFn].times = f->m_times + TIME_PRESSURE;
	
	int i;
	for (i=0; i<in_iterations; i++)
		mpCTaskAdd(f->r_coherence, curFn, -1, 1, 1);
	
	f->m_usedFunctions = curFn+1;
	
	curFn = f->m_usedFunctions;
	errorAssert(curFn<MAX_FNS, error_memory, "Too many different tasks!");
	
	f->m_fns[curFn].fn = fluid_applyPressure;
	f->m_fns[curFn].mode.pressure.velX = f->r_velocityX;
	f->m_fns[curFn].mode.pressure.velY = f->r_velocityY;
	f->m_fns[curFn].mode.pressure.pressure = f->r_pressure;
	f->m_fns[curFn].times = f->m_times + TIME_PRESSURE;
	
	mpCTaskAdd(f->r_coherence, curFn, -1, 1, 1);
	
	f->m_usedFunctions = curFn+1;
}

void fluidTaskViscosity(fluid *f, int in_iterations)
{
	if (f->m_viscosity < 0.001f)
		return;
	
	int curFn = f->m_usedFunctions;
	errorAssert(curFn<MAX_FNS, error_memory, "Too many different tasks!");
	
	float viscocity = f->m_viscosity;
	
	f->m_fns[curFn].fn = fluid_viscosity;
	f->m_fns[curFn].mode.viscosity.velX = f->r_velocityX;
	f->m_fns[curFn].mode.viscosity.velY = f->r_velocityY;
	f->m_fns[curFn].mode.viscosity.alpha = 1.0f / viscocity * f->m_timestep;
	f->m_fns[curFn].mode.viscosity.beta = 1.0f / (1.0f / viscocity * f->m_timestep  + 4.0f);
	f->m_fns[curFn].times = f->m_times + TIME_VISCOSITY;
	
	int i;
	for (i=0; i<in_iterations; i++)
		mpCTaskAdd(f->r_coherence, curFn, -1, 1, 1);
	
	f->m_usedFunctions = curFn+1;
}


void fluidTaskVorticity(fluid *f)
{
	if (f->m_vorticity < 0.001f)
		return;
	
	int curFn = f->m_usedFunctions;
	errorAssert(curFn<MAX_FNS, error_memory, "Too many different tasks!");
	
	f->m_fns[curFn].fn = fluid_vorticity_curl;
	f->m_fns[curFn].mode.vorticity.velX = f->r_velocityX;
	f->m_fns[curFn].mode.vorticity.velY = f->r_velocityY;
	f->m_fns[curFn].mode.vorticity.z = f->r_tmpVelX;
	f->m_fns[curFn].mode.vorticity.e = f->m_vorticity;
	f->m_fns[curFn].times = f->m_times + TIME_VORTICITY;
	
	mpCTaskAdd(f->r_coherence, curFn, -1, 1, 1);
	
	f->m_usedFunctions = curFn+1;
	
	curFn = f->m_usedFunctions;
	errorAssert(curFn<MAX_FNS, error_memory, "Too many different tasks!");
	
	f->m_fns[curFn].fn = fluid_vorticity_apply;
	f->m_fns[curFn].mode.vorticity.velX = f->r_velocityX;
	f->m_fns[curFn].mode.vorticity.velY = f->r_velocityY;
	f->m_fns[curFn].mode.vorticity.z = f->r_tmpVelX;
	f->m_fns[curFn].mode.vorticity.e = f->m_vorticity;
	f->m_fns[curFn].times = f->m_times + TIME_VORTICITY;
	
	mpCTaskAdd(f->r_coherence, curFn, -1, 1, 1);
	
	f->m_usedFunctions = curFn+1;
}


void fluidTaskDampen(fluid *f, field *dst, float amt)
{
	if (f->m_timestep < 0.0001f || amt < 0.0001f ||
		(amt > 0.999f && amt < 1.001f))
		return;
	
	int curFn = f->m_usedFunctions;
	errorAssert(curFn < MAX_FNS, error_memory, "Too many different tasks!");
	
	f->m_fns[curFn].fn = fluid_dampen;
	f->m_fns[curFn].mode.dampen.f = dst;
	f->m_fns[curFn].mode.dampen.e = pow(amt, f->m_timestep);
	f->m_fns[curFn].times = NULL;
	mpCTaskAdd(f->r_coherence, curFn, 0,0,0);
	
	f->m_usedFunctions = curFn+1;
}


//Called on each processor to do a specified amount of work.
void fluidMP(void *in_o)
{
	fluid *o = (fluid*)in_o;
	mpCoherence *c = o->r_coherence;
	
	int tid, fn, tsk;
	mpCTaskObtain(c, &tid, &fn, &tsk);
	while (tid != -1)
	{
		//Execute the desired function from the list of functions...
		o->m_fns[fn].fn(o, tsk, &o->m_fns[fn].mode);
	
		//Fetch another function!
		mpCTaskComplete(c, tid, fn, tsk,
								&tid, &fn, &tsk);
	}
	
	
	mpQueuePush(o->r_blocker, NULL);
}

//Version of fluidMP for debugging (eg. it captures timing information)
void fluidTimedMP(void *in_o)
{
	fluid *o = (fluid*)in_o;
	mpCoherence *c = o->r_coherence;
	
	int tid, fn, tsk;
	
	double curTime = x_time();
	mpCTaskObtain(c, &tid, &fn, &tsk);
	while (tid != -1)
	{
		//Execute the desired function from the list of functions...
		o->m_fns[fn].fn(o, tsk, &o->m_fns[fn].mode);
		
		double nextTime = x_time();
		if (o->m_fns[fn].times != NULL)
		{
			AtomicAdd32Barrier(*o->m_fns[fn].times, (int)((nextTime-curTime)*1000000));
		}
		curTime = nextTime;
		
		//Fetch another function!
		mpCTaskComplete(c, tid, fn, tsk,
						&tid, &fn, &tsk);
		
		nextTime = x_time();
		AtomicAdd32Barrier(o->m_times[TIME_TASKSCHED], (int)((nextTime-curTime)*1000000));
		curTime = nextTime;
	}
	
	
	mpQueuePush(o->r_blocker, NULL);
}

//NOTE for 512x512 with 40 iterations for fairness:
//	4.5 FPS on Intel using old program	(0% improvment)
//	5.0 FPS on Intel using new			(13% improvement)
//	5.5 FPS on Intel using new x64 SL	(18% improvement)

//NOTE for 512x512 with 40 iterations for fairness
//	4.5 FPS on PPC x4 using old program
//	5.0 FPS on PPC x4 using new program		(13% improvement)

//Called every frame to advance the fluid simulation...
void fluidAdvance(fluid *in_f)
{
	//Add in the basic fluid simulation as it was before - except with SIMPLE
	//boundary conditions
	fluidTaskVorticity(in_f);
	fluidTaskViscosity(in_f, 20);
	
	if (in_f->flags & FLUID_SIMPLEFREE)
		fluidTaskPressure(in_f, 20, in_f->r_density);
	else
		fluidTaskPressure(in_f, 20, NULL);
	
	fluidTaskDampen(in_f, in_f->r_density, in_f->m_fadeDens);
	fluidTaskDampen(in_f, in_f->r_velocityX, in_f->m_fadeVel);
	fluidTaskDampen(in_f, in_f->r_velocityY, in_f->m_fadeVel);
	
	fluidTaskAddForwardAdvection(in_f);
	fluidTaskAddBackwardAdvection(in_f);
	fluidTaskCorrectorRepos(in_f);
	fluidTaskAdvectDensity(in_f);
	
	//We just need to run the tasks that have already been setup...
	int spawned;
	if (in_f->flags & FLUID_TIMERS)
	{
		int i;
		for (i=0; i<TIME_TOTAL; i++)
			in_f->m_times[i] = 0;
		spawned = mpTaskFlood(fluidTimedMP, in_f);
	}
	else
		spawned = mpTaskFlood(fluidMP, in_f);
	
	int i;
	for (i=0; i<spawned; i++)
		mpQueuePop(in_f->r_blocker);
	
	//Clear the scheduler (for the next pass)
	mpCReset(in_f->r_coherence);
	in_f->m_usedFunctions = 0;
	
	//fluidSwap(field*, in_f->r_density, in_f->r_density_swap);
}


field *fluidDensity(fluid *in_f)
{
	return in_f->r_density;
}

field *fluidMovedDensity(fluid *in_f)
{
	return in_f->r_density_swap;
}

field *fluidVelocityX(fluid *in_f)
{
	return in_f->r_velocityX;
}

field *fluidVelocityY(fluid *in_f)
{
	return in_f->r_velocityY;
}

float fluidAdvectionTime(fluid *f)
{
	return (float)f->m_times[TIME_ADVECTION]/1000000;
}

float fluidPressureTime(fluid *f)
{
	return (float)f->m_times[TIME_PRESSURE]/1000000;
}

float fluidViscosityTime(fluid *f)
{
	return (float)f->m_times[TIME_VISCOSITY]/1000000;
}

float fluidVorticityTime(fluid *f)
{
	return (float)f->m_times[TIME_VORTICITY]/1000000;
}

float fluidThreadSchedulerTime(fluid *f)
{
	return (float)f->m_times[TIME_TASKSCHED]/1000000;
}


