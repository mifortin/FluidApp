/*
 *  Test_Pipeline.c
 *  FluidApp
*
 */

#include "memory.h"
#include <stdlib.h>
#include <math.h>

#define DS (16)
#define BLOCK 8

#define TYPE int

static TYPE *data;
static TYPE *d2;

void initData()
{
	int i;
	for (i=0; i<DS; i++)
		data[i] = i + 1;
}

void initData2()
{
	int i;
	for (i=0; i<DS/4; i++)
	{
		data[4*i+0] = i + 1;
		data[4*i+1] = DS/4 + i + 1;
		data[4*i+2] = 2*DS/4 + i + 1;
		data[4*i+3] = 3*DS/4 + i + 1;
	}
}

//Normally there are multiple integer units - let's see if we can show that
//they exist...
static void simpleSum()
{
	int i;
	TYPE p1;
	p1 = data[0];
	for (i=1; i<DS; i++)
	{
		data[i] += p1;
		p1 = data[i];
	}
}

static void parallelSum()
{
	int i;
	TYPE p1,p2,p3,p4,min;
	
	min = 0;
	
	int k;
	for (k=0; k<DS/4;)
	{
		p1 = data[k+0];
		p2 = data[k+1];
		p3 = data[k+2];
		p4 = data[k+3];
		for (i=1 + k; i<BLOCK/4+k; i++)
		{
			data[4*i] += p1;
			data[4*i+1] += p2;
			data[4*i+2] += p3;
			data[4*i+3] += p4;
			
			p1 = data[4*i];
			p2 = data[4*i+1];
			p3 = data[4*i+2];
			p4 = data[4*i+3];
		}
		
		p1 = data[4*k+BLOCK-4] + data[4*k+BLOCK-3] + data[4*k+BLOCK-2] + min;
		p2 = data[4*k+BLOCK-4] + data[4*k+BLOCK-3] + min;
		p3 = data[4*k+BLOCK-4] + min;
		for (i=0 + k; i<BLOCK/4+k; i++)
		{
			data[4*i + 3] += p1;
			data[4*i + 2] += p2;
			data[4*i + 1] += p3;
		}
		
		min = data[4*(i-1) + 3];
		
		k = k+BLOCK/4;
	}
}

void testPipeline()
{
	data = malloc(sizeof(TYPE) * DS);
	d2 = malloc(sizeof(TYPE) *DS);
	
	int j;
	for (j=0; j<10;j++)
	{
		int k;
		initData();
		double t1 = x_time();
		for (k=0; k<1; k++)
			simpleSum();
		printf("Time: Simple: %f \n", x_time() - t1);
		
		TYPE *t = data;
		data = d2;
		d2 = t;
		
		initData2();
		t1 = x_time();
		for (k=0; k<1; k++)
			parallelSum();
		printf("Time: Parallel: %f \n", x_time() - t1);
	}
	
	int i;
	for (i = 0; i<DS; i++)
	{
		if (fabs(data[i] - d2[i]) > 0.1f)
			break;
	}
	
	if (i != DS)
	{
		printf("DIFFERENT RESULTS\n");
		for (i=0; i<DS; i++)
			printf("%3i ", i);
		printf("\n");
		for (i=0; i<DS/4; i++)
			printf("%3i %3i %3i %3i ", data[i], data[i+DS/4], data[i+2*DS/4], data[i+3*DS/4]);
		printf("\n\n");
		for (i=0; i<DS; i++)
			printf("%3i ", i);
		printf("\n");
		for (i=0; i<DS; i++)
			printf("%3i ", d2[i]);
		printf("\n");/**/
	}
	
	free(data);
	free(d2);
}
