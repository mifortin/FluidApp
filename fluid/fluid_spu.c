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
static const vector unsigned char shLeft =
	{
		0x04,	0x05,	0x06,	0x07,	0x08,	0x09,	0x0A,	0x0B,
		0x0C,	0x0D,	0x0E,	0x0F,	0x10,	0x11,	0x12,	0x13
	};

static const vector unsigned char shRight =
	{
		0x0C,	0x0D,	0x0E,	0x0F,	0x10,	0x11,	0x12,	0x13,
		0x14,	0x15,	0x16,	0x17,	0x18,	0x19,	0x1A,	0x1B
	};



//Shuffle to duplicate left-most and right most element, eg:
//
//	origin			A B C D
//	dupLeft			B B C D
//	dupRight		A B C C
static const vector unsigned char dupLeft =
	{
		0x04,	0x05,	0x06,	0x07,	0x04,	0x05,	0x06,	0x07,
		0x08,	0x09,	0x0A,	0x0B,	0x0C,	0x0D,	0x0E,	0x0F
	};

static const vector unsigned char dupRight =
	{
		0x00,	0x01,	0x02,	0x03,	0x04,	0x05,	0x06,	0x07,
		0x08,	0x09,	0x0A,	0x0B,	0x08,	0x09,	0x0A,	0x0B
	};


//Zero is used quite often...
static const vector float zero = {0,0,0,0};

//Used to negate either the first or last element
static const vector float nFirst = {-1, 1, 1, 1};
static const vector float nLast = {1, 1, 1, -1};


void genViscosityBorder(int w,	vector float *vVelX_Dest,
								vector float *vVelY_Dest,
								vector float *vVelX,
								vector float *vVelY)
{
	int x;
	for (x=0; x<w; x++)
	{
		vVelX_Dest[x] = spu_sub(zero, vVelX[x]);
		vVelY_Dest[x] = spu_sub(zero, vVelY[x]);
	}
}


void genViscosity(int w,		vector float vAlpha,
								vector float vBeta,
								vector float *vVelX,
								vector float *vVelY,
								vector float *vVelXN,
								vector float *vVelYN,
								vector float *vVelXP,
								vector float *vVelYP)
{
	int x;
	
	vector float slX, srX, tmp1X, tmp2X, tmp3X;
	vector float slY, srY, tmp1Y, tmp2Y, tmp3Y;
	
	tmp1X = spu_madd(vVelX[0], vAlpha, vVelXN[0]);
	tmp2X = spu_add(vVelXP[0], tmp1X);
	
	tmp1Y = spu_madd(vVelY[0], vAlpha, vVelYN[0]);
	tmp2Y = spu_add(vVelYP[0], tmp1Y);
	
	slX = spu_shuffle(vVelX[0], vVelX[1], shLeft);
	srX = spu_shuffle(zero, vVelX[0], shRight);
	
	slY = spu_shuffle(vVelY[0], vVelY[1], shLeft);
	srY = spu_shuffle(zero, vVelY[0], shRight);
	
	tmp3X = spu_add(slX, srX);
	tmp1X = spu_add(tmp3X, tmp2X);
	
	tmp3Y = spu_add(slY, srY);
	tmp1Y = spu_add(tmp3Y, tmp2Y);
	
	vVelX[0] = spu_mul(nFirst, spu_shuffle(spu_mul(tmp1X, vBeta), zero, dupLeft));
	vVelY[0] = spu_mul(nFirst, spu_shuffle(spu_mul(tmp1Y, vBeta), zero, dupLeft));
	
	for (x=1; x<w-1; x++)
	{
		tmp1X = spu_madd(vVelX[x], vAlpha, vVelXN[x]);
		tmp2X = spu_add(vVelXP[x], tmp1X);
		
		tmp1Y = spu_madd(vVelY[x], vAlpha, vVelYN[x]);
		tmp2Y = spu_add(vVelYP[x], tmp1Y);
		
		slX = spu_shuffle(vVelX[x], vVelX[x+1], shLeft);
		srX = spu_shuffle(vVelX[x-1], vVelX[x], shRight);
		
		slY = spu_shuffle(vVelY[x], vVelY[x+1], shLeft);
		srY = spu_shuffle(vVelY[x-1], vVelY[x], shRight);
		
		tmp3X = spu_add(slX, srX);
		tmp1X = spu_add(tmp3X, tmp2X);
		
		tmp3Y = spu_add(slY, srY);
		tmp1Y = spu_add(tmp3Y, tmp2Y);
		
		vVelX[x] = spu_mul(tmp1X, vBeta);
		vVelY[x] = spu_mul(tmp1Y, vBeta);
	}
	
	tmp1X = spu_madd(vVelX[x], vAlpha, vVelXN[x]);
	tmp2X = spu_add(vVelXP[x], tmp1X);
	
	tmp1Y = spu_madd(vVelY[x], vAlpha, vVelYN[x]);
	tmp2Y = spu_add(vVelYP[x], tmp1Y);
	
	slX = spu_shuffle(vVelX[x], zero, shLeft);
	srX = spu_shuffle(vVelX[x-1], vVelX[x], shRight);
	
	slY = spu_shuffle(vVelY[x], zero, shLeft);
	srY = spu_shuffle(vVelY[x-1], vVelY[x], shRight);
	
	tmp3X = spu_add(slX, srX);
	tmp1X = spu_add(tmp3X, tmp2X);
	
	tmp3Y = spu_add(slY, srY);
	tmp1Y = spu_add(tmp3Y, tmp2Y);
	
	vVelX[x] = spu_mul(nLast, spu_shuffle(spu_mul(tmp1X, vBeta), zero, dupRight));
	vVelY[x] = spu_mul(nLast, spu_shuffle(spu_mul(tmp1Y, vBeta), zero, dupRight));
}


void genPressureBorder(int w,	vector float *vPressureDest,
								vector float *vPressureSrc)
{
	int x;
	for (x=0; x<w; x++)
	{
		vPressureDest[x] = vPressureSrc[x];
	}
}


void genPressure(int w,	vector float *vPressure,
						vector float *vPressureN,
						vector float *vPressureP,
						vector float *vVelX,
						vector float *vVelYN,
						vector float *vVelYP)
{
	vector float div4 = {0.25f, 0.25f, 0.25f, 0.25f};
		
	vector float tmp1, tmp2, tmp3, tmp4;

	vector float sl_p = spu_shuffle(vPressure[0], vPressure[1], shLeft);
	vector float sr_p = spu_shuffle(zero, vPressure[0], shRight);
	
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
	for (x=1; x<w-1; x++)
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


void pressureApply(int w,	vector float *vVelX,
							vector float *vVelY,
							vector float *vPressure,
							vector float *vPressureN,
							vector float *vPressureP)
{
	vector unsigned int leftMask = {0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF};
	vector unsigned int rightMask = {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x00000000};
	
	vector float sl = spu_shuffle(vPressure[0], vPressure[1], shLeft);
	vector float sr = spu_shuffle(zero, vPressure[0], shRight);
	
	vector float tmpX = spu_sub(sl, sr);
	tmpX = spu_and(tmpX, (vector float)leftMask);
	
	vector float tmpY = spu_sub(vPressureN[0], vPressureP[0]);
	tmpY = spu_and(tmpY, (vector float)leftMask);
	vVelX[0] = spu_sub(vVelX[0], tmpX);
	vVelY[0] = spu_sub(vVelY[0], tmpY);
	
	int x;
	for (x=1; x<w-1; x++)
	{
		sl = spu_shuffle(vPressure[x], vPressure[x+1], shLeft);
		sr = spu_shuffle(vPressure[x-1], vPressure[x], shRight);
		
		tmpX = spu_sub(sl, sr);
		tmpY = spu_sub(vPressureN[x], vPressureP[x]);
		vVelX[x] = spu_sub(vVelX[x], tmpX);
		vVelY[x] = spu_sub(vVelY[x], tmpY);
	}
	
	sl = spu_shuffle(vPressure[x], zero, shLeft);
	sr = spu_shuffle(vPressure[x-1], vPressure[x], shRight);
	
	tmpX = spu_sub(sl, sr);
	tmpX = spu_and(tmpX, (vector float)rightMask);
	tmpY = spu_sub(vPressureN[x], vPressureP[x]);
	tmpY = spu_and(tmpY, (vector float)rightMask);
	vVelX[x] = spu_sub(vVelX[x], tmpX);
	vVelY[x] = spu_sub(vVelY[x], tmpY);
}


int main(unsigned long long spu_id __attribute__ ((unused)), unsigned long long argv)
{
	genPressure(480,
				&dat[0][0].vf, &dat[0][1].vf, &dat[0][2].vf,
				&dat[0][3].vf, &dat[0][4].vf, &dat[0][5].vf);

	return 0;
}
