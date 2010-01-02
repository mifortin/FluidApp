/*
 *  gpgpu_program.c
 *  FluidApp
 */

#include "gpgpu_pvt.h"
#include <string.h>

#ifdef MP_OPENCL

void GPUProgramRelease(void *in_o)
{
	GPUProgram *o = (GPUProgram*)in_o;
	
	if (o->r_execution)		clReleaseKernel(o->r_execution);
	if (o->r_binary)		clReleaseProgram(o->r_binary);
}

GPUProgram *GPUProgramCreate(char *in_szSource, unsigned int in_flags)
{
	GPUProgram *r = x_malloc(sizeof(GPUProgram), GPUProgramRelease);
	memset(r, 0, sizeof(GPUProgram));
	
	char *szSource;
	char szStackSource[1024*20];		//20k file - big enough!
	if (in_flags & GPUPROGRAM_FROM_FILE)
	{
		szSource = szStackSource;
		
		FILE *src = x_fopen(in_szSource, "rb");
		size_t read = fread(szSource, sizeof(char), 1024*20, src);
		if (read >= 1024*20) read = 1024*20-1;
		szSource[read] = '\0';
		fclose(src);
	}
	else
		szSource = in_szSource;
	
	cl_context context = GPGPU_OpenCLContext_pvt();
	cl_device_id device = GPGPU_OpenCLDevice_pvt();
	cl_int err;
	r->r_binary = clCreateProgramWithSource(	context,
												1,
												(const char**)&szSource,
												NULL,
												&err);
	errorAssert(err == CL_SUCCESS, error_create,
				"Failed creating program (%i)!\n", err);
	
	err = clBuildProgram(	r->r_binary, 1, &device,
							"-cl-single-precision-constant "
							"-cl-denorms-are-zero "
							"-cl-strict-aliasing "
							"-cl-mad-enable "
							"-cl-no-signed-zeros "
							"-cl-fast-relaxed-math ",
							NULL, NULL);
	if (in_flags & GPUPROGRAM_FROM_FILE)
		printf("Compiling %s; results:\n", in_szSource);
	else
		printf("Compile results:\n");
	
	if (err == CL_SUCCESS)
		printf(" - SUCCESS!");
	
	cl_int err2;
	err2 = clGetProgramBuildInfo(	r->r_binary,
									device,
									CL_PROGRAM_BUILD_LOG,
									1024*20,
									szStackSource,
									NULL);
	errorAssert(err2 == CL_SUCCESS, error_create,
				"Failed querying build status (%i)!\n", err);
	printf("%s\n",szStackSource);
	
	errorAssert(err == CL_SUCCESS, error_create,
				"Failed building program (%i)!\n", err);
	
	r->r_execution = clCreateKernel(r->r_binary, "main", &err);
	
	errorAssert(err == CL_SUCCESS, error_create,
				"Failed preparing executable for main function (%i)!\n", err);
	
}

#else

GPUProgram *GPUProgramCreate(char *in_szSource, unsigned int in_flags)
{
	return NULL;
}

#endif
