/*
 *  fluid_spu.c
 *  
 */

#include <spu_mfcio.h>
#include "fluid_spu.h"

//Buffers (for working...)
union bufferData
{
	float f[4];
	int i[4];
	unsigned char ch[16];
	
	vector float vf;
	vector signed int vi;
	vector unsigned char vc;
};

volatile union bufferData dat[33][480] __attribute__ ((aligned (128)));



int main(unsigned long long spu_id __attribute__ ((unused)), unsigned long long argv)
{
	return 0;
}
