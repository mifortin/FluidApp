/*
 *  gpgpu_pvt.h
 *  FluidApp
 *
 */

#include "gpgpu.h"
#include "error.h"
#include "memory.h"

#ifdef MP_OPENCL
#include <OpenCL/cl.h>

struct GPUField
{
	cl_mem r_handle;
};

struct GPUProgram
{
	cl_program	r_binary;		//Binary (program)
	cl_kernel	r_execution;	//Execution environment (kernels, etc.)
};

//Interop functions
cl_context GPGPU_OpenCLContext_pvt();
cl_device_id GPGPU_OpenCLDevice_pvt();

#endif
