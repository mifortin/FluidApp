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

union bufferData dat[FLUID_BUFFERS][MAX_WIDTH/4] __attribute__ ((aligned (128)));

fluid_context context;

//Notes on shifting (implemented using spu_shuffle)
//
//	orig		A B C D  E F G H  I J K L
//	left		         F G H I
//	right		         D E F G

void genPressure(int w,	vector float *vPressure,
						vector float *vPressureN,
						vector float *vPressureP,
						vector float *vVelX,
						vector float *vVelYN,
						vector float *vVelYP)
{
	vector float div4 = {0.25f, 0.25f, 0.25f, 0.25f};
	vector float zero = {0,0,0,0};

	vector unsigned char shLeft =
		{
			0x04,	0x05,	0x06,	0x07,	0x08,	0x09,	0x0A,	0x0B,
			0x0C,	0x0D,	0x0E,	0x0F,	0x10,	0x11,	0x12,	0x13
		};
	
	vector unsigned char shRight =
		{
			0x0C,	0x0D,	0x0E,	0x0F,	0x10,	0x11,	0x12,	0x13,
			0x14,	0x15,	0x16,	0x17,	0x18,	0x19,	0x1A,	0x1B
		};
	
	vector unsigned char dupLeft =
		{
			0x04,	0x05,	0x06,	0x07,	0x04,	0x05,	0x06,	0x07,
			0x08,	0x09,	0x0A,	0x0B,	0x0C,	0x0D,	0x0E,	0x0F
		};
	
	vector unsigned char dupRight =
		{
			0x00,	0x01,	0x02,	0x03,	0x04,	0x05,	0x06,	0x07,
			0x08,	0x09,	0x0A,	0x0B,	0x08,	0x09,	0x0A,	0x0B
		};
		
	vector float tmp1, tmp2, tmp3, tmp4;

	vector float sl_p = spu_shuffle(vPressure[0], vPressure[1], shLeft);
	vector float sr_p = spu_shuffle(zero, vPressure[0], shRight);
	
	vector float sl_p_2, sr_p_2, sl_vx_2, sr_vx_2, tmp1_2, tmp2_2, tmp3_2, tmp4_2;
	
	vector float sl_vx = spu_shuffle(vVelX[0], vVelX[1], shLeft);
	vector float sr_vx = spu_shuffle(zero, vVelX[0], shRight);
	
	tmp1 = spu_add(sl_p, sr_p);
	tmp2 = spu_add(vPressureP[0], vPressureN[0]);
	tmp3 = spu_sub(sr_vx, sl_vx);
	tmp4 = spu_sub(vVelYP[0], vVelYN[0]);
	
	tmp1 = spu_add(tmp1, tmp2);
	tmp3 = spu_add(tmp3, tmp4);
	tmp1 = spu_add(tmp1, tmp3);
	
	vPressure[0] = spu_mul(tmp1, div4);

	int x;
	for (x=0; x<w-2; x+=2)
	{
		sl_p = spu_shuffle(vPressure[x], vPressure[x+1], shLeft);
		sr_p = spu_shuffle(vPressure[x-1], vPressure[x], shRight);
		
		sl_p_2 = spu_shuffle(vPressure[x+1], vPressure[x+2], shLeft);
		sr_p_2 = spu_shuffle(vPressure[x], vPressure[x+1], shRight);
		
		sl_vx = spu_shuffle(vVelX[x], vVelX[x+1], shLeft);
		sr_vx = spu_shuffle(vVelX[x-1], vVelX[x], shRight);
		
		sl_vx_2 = spu_shuffle(vVelX[x+1], vVelX[x+2], shLeft);
		sr_vx_2 = spu_shuffle(vVelX[x], vVelX[x+1], shRight);
		
		tmp1 = spu_add(sl_p, sr_p);
		tmp2 = spu_add(vPressureP[x], vPressureN[x]);
		tmp3 = spu_sub(sr_vx, sl_vx);
		tmp4 = spu_sub(vVelYP[x], vVelYN[x]);
		
		tmp1_2 = spu_add(sl_p_2, sr_p_2);
		tmp2_2 = spu_add(vPressureP[x+1], vPressureN[x+1]);
		tmp3_2 = spu_sub(sr_vx_2, sl_vx_2);
		tmp4_2 = spu_sub(vVelYP[x+1], vVelYN[x+1]);
		
		tmp1 = spu_add(tmp1, tmp2);
		tmp3 = spu_add(tmp3, tmp4);
		tmp1 = spu_add(tmp1, tmp3);
		
		vPressure[x] = spu_mul(tmp1, div4);
		
		tmp1_2 = spu_add(tmp1_2, tmp2_2);
		tmp3_2 = spu_add(tmp3_2, tmp4_2);
		tmp1_2 = spu_add(tmp1_2, tmp3_2);
		
		vPressure[x+1] = spu_mul(tmp1_2, div4);
	}
	for (; x<w-1; x++)
	{
		sl_p = spu_shuffle(vPressure[x], vPressure[x+1], shLeft);
		sr_p = spu_shuffle(vPressure[x-1], vPressure[x], shRight);
		
		sl_vx = spu_shuffle(vVelX[x], vVelX[x+1], shLeft);
		sr_vx = spu_shuffle(vVelX[x-1], vVelX[x], shRight);
		
		tmp1 = spu_add(sl_p, sr_p);
		tmp2 = spu_add(vPressureP[x], vPressureN[x]);
		tmp3 = spu_sub(sr_vx, sl_vx);
		tmp4 = spu_sub(vVelYP[x], vVelYN[x]);
		
		tmp1 = spu_add(tmp1, tmp2);
		tmp3 = spu_add(tmp3, tmp4);
		tmp1 = spu_add(tmp1, tmp3);
		
		vPressure[x] = spu_mul(tmp1, div4);
	}
	
	sl_p = spu_shuffle(vPressure[x], zero, shLeft);
	sr_p = spu_shuffle(vPressure[x-1], vPressure[x], shRight);
	
	sl_vx = spu_shuffle(vVelX[x], zero, shLeft);
	sr_vx = spu_shuffle(vVelX[x-1], vVelX[x], shRight);
	
	tmp1 = spu_add(sl_p, sr_p);
	tmp2 = spu_add(vPressureP[x], vPressureN[x]);
	tmp3 = spu_sub(sr_vx, sl_vx);
	tmp4 = spu_sub(vVelYP[x], vVelYN[x]);
	
	tmp1 = spu_add(tmp1, tmp2);
	tmp3 = spu_add(tmp3, tmp4);
	tmp1 = spu_add(tmp1, tmp3);

	vPressure[0] = spu_shuffle(vPressure[0], zero, dupLeft);
	
	vPressure[x] = spu_shuffle(spu_mul(tmp1, div4), zero, dupRight);
}


int main(unsigned long long spu_id __attribute__ ((unused)), unsigned long long argv)
{
	genPressure(480,
				&dat[0][0].vf, &dat[0][1].vf, &dat[0][2].vf,
				&dat[0][3].vf, &dat[0][4].vf, &dat[0][5].vf);

	return 0;
}
