/*
 *  fluid_spu.c
 *  
 */

#include <spu_mfcio.h>
#include "fluid_spu.h"

#define MAX 9

//Buffers (for working...)
union bufferItem
{
	float f[4];
	int i[4];
	unsigned char ch[16];
	
	vector float vf;
	vector signed int vi;
	vector unsigned char vc;
};

static int onoff = 0;

typedef struct bufferData { union bufferItem d[MAX_WIDTH/4]; } bufferData;
typedef struct bufferMap  {	union bufferItem* d;} bufferMap;

bufferMap  dat[FLUID_BUFFERS];

bufferData realData[FLUID_BUFFERS] __attribute__ ((aligned (128)));

fluid_context context __attribute__ ((aligned(16)));


//DMA operations.
//	We have a fairly good idea of the DMA requirements of the system.  We
//	have a range...
//
//		We can load up to 3 buffers and write out up to 3 buffers once they
//		are done.
//
//		First - fill in the DMA info...
struct {
	char *ptr;		//Source pointer (assumed packed)
	char *dst;		//Destination	 (assumed packed)
	int buffLen;	//Width (in bytes)
} DMA[3];


//Second - init a series of operations to load up some data...
static void dmaLoad(int in_row, int m_row, int l)		//Internal row / extenral row
{
	int tag = 1;
	int i;	
	
	//printf("DMA LOAD %i into %i (%i)\n", m_row, in_row,DMA[0].buffLen);
	
	for (i=0; i < 3; i++)
	{
		if (!DMA[i].ptr)
			continue;
		
		mfc_get((void*)dat[i+in_row*3].d,
				(unsigned int)(DMA[i].ptr + DMA[i].buffLen*m_row),
				DMA[i].buffLen*l, tag, 0, 0);
	}
}

//Third - ensure the data is read in...
static void dmaComplete()
{
	int tag = 1;
	mfc_write_tag_mask(1 << tag);
	mfc_read_tag_status_all();
}

//Write out data...
static void dmaStore(int in_row, int m_row, int l)
{
	int tag = 1;
	int i;	
	
	//printf("DMA STORE %i from %i (%i) length %i\n", m_row, in_row,DMA[0].buffLen, l);
	
	for (i=0; i < 3; i++)
	{
		if (!DMA[i].dst)
			continue;
		
		mfc_put((void*)dat[i+in_row*3].d,
				(unsigned int)(DMA[i].dst + DMA[i].buffLen*m_row),
				DMA[i].buffLen*l, tag, 0, 0);
	}
	
	//printf("OK!\n");
}


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


static void genViscosityBorder(int w,	vector float *vVelX_Dest,
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


static void genViscosity(int w,		vector float vAlpha,
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


void genPressureBorder(const int w,
								vector float *vPressureDest,
								const vector float *vPressureSrc)
{
	int x;
	for (x=0; x<w; x++)
	{
		vPressureDest[x] = vPressureSrc[x];
	}
}


static void genPressure(const int w,
						vector float *vPressure,
						const vector float *vPressureN,
						const vector float *vPressureP,
						const vector float *vVelX,
						const vector float *vVelYN,
						const vector float *vVelYP)
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


static void pressureApply(const int w,
							vector float *vVelX,
							vector float *vVelY,
							const vector float *vPressure,
							const vector float *vPressureN,
							const vector float *vPressureP)
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


static void processorBorder(int edge, int inside)
{
	if (context.cmd == CMD_PRESSURE)
		genPressureBorder(context.width/4, &dat[edge].d[0].vf, &dat[inside].d[0].vf);
	else if (context.cmd == CMD_VISCOSITY)
		genViscosityBorder(context.width/4, &dat[edge].d[0].vf, &dat[edge].d[1].vf,
											&dat[inside].d[0].vf, &dat[inside].d[1].vf);
}

static void processRow(int r)
{
	if (context.cmd == CMD_PRESSURE)
		genPressure(	context.width/4,
						&dat[3*r].d[0].vf,
						&dat[3*(r+1)].d[0].vf,
						&dat[3*(r-1)].d[0].vf,
						&dat[3*r+1].d[0].vf,
						&dat[3*(r+1)+2].d[0].vf,
						&dat[3*(r-1)+2].d[0].vf);
	else if (context.cmd == CMD_VISCOSITY)
		genViscosity(	context.width/4,
						(vector float){context.alpha,context.alpha,context.alpha,context.alpha},
						(vector float){context.beta,context.beta,context.beta,context.beta},
						&dat[r].d[0].vf, &dat[r].d[1].vf,
						&dat[r+1].d[0].vf, &dat[r+1].d[1].vf,
						&dat[r-1].d[0].vf, &dat[r-1].d[1].vf);
}


void forward()
{
	int i;
	int cLeft = 1;
	int cEnd = MAX-2;
	
	int storeStart = -1;
	int storeEnd = -1;
	
	while (cLeft <= cEnd)
	{
		//Loops over	1...9
		//				2...8
		//				3...7
		//				4...6
		//				  5
		for (i=cLeft; i<=cEnd; i++)
		{				
			if (i+context.start < 0)
				continue;
			
			//OK!  we got a row!
			if (i+context.start > 0 && i+context.start < context.height-1)
			{
				processRow(i);
			}
			else if (i+context.start == 0)
			{
				processorBorder(i, i+1);
			}
			else if (i+context.start == context.height-1)
			{
				processorBorder(i, i-1);
			}
		}
		
		//Write what we need back out to main memory... (for 16 mem ops, 25 ops!)
		if (cLeft + context.start >= 0 && cLeft + context.start < context.height)
		{
			if (storeStart == -1)
				storeStart = cLeft;
			
			storeEnd = cLeft;
			
			if (storeEnd - storeStart >= 3)
			{
				dmaStore(cLeft, cLeft + context.start,1+storeEnd-storeStart);
				
				storeEnd = -1;
				storeStart = -1;
			}
		}
		
		cLeft++;
		cEnd--;
	}
	
	if (storeStart != -1)
		dmaStore(storeStart, storeStart + context.start, 1+storeEnd-storeStart);

//Don't need, catch on next round...	
//	dmaComplete();
}


//In reverse, we store rather than load a lot of data...
void reverse()
{
	int i;
	int cLeft = MAX/2;
	int cEnd = MAX/2;
	
	while (cLeft >= 1)
	{	
		//Loops over	1...9
		//				2...8
		//				3...7
		//				4...6
		//				  5
		for (i=cLeft; i<=cEnd; i++)
		{
			//printf("SPU: %i [%i,%i]\n", i, cLeft, cEnd);
				
			if (i+context.start < 0)
				continue;
			
			//OK!  we got a row!
			if (i+context.start > 0 && i+context.start < context.height-1)
			{
				processRow(i);
			}
			else if (i+context.start == 0)
			{
				processorBorder(i, i+1);
			}
			else if (i+context.start == context.height-1)
			{
				processorBorder(i, i-1);
			}
			
			if (cLeft == 1
				&& i+context.start >= 0
				&& i+context.start < context.height)
			{				
				if (i < MAX)
					dmaStore(i, i+context.start,1);
			}
		}
		
		cLeft--;
		cEnd++;
	}
}


int main(unsigned long long spu_id __attribute__ ((unused)), unsigned long long argv)
{
	//Load up basic data...
	//printf("SPU %i entering main function (%i %i)\n", spu_id, (int)argv, sizeof(context));
	
	//printf("Queues: %i\n", (int)mfc_stat_cmd_queue());
	
	int tag = 1;
	mfc_get((void*)&context, (unsigned int)argv, sizeof(context), tag, 0,0);
	
	//printf("SPU now waiting for instructions\n");
	
	int i;
	mfc_write_tag_mask(1 << tag);
	mfc_read_tag_status_all();
	
	//printf("SPU here running command %i\n", context.cmd);
	
	//printf("SPU completed DMA\n");

	//printf("SPU CONTEXT WIDTH: %i\n", context.width);
	//printf("SPU CONTEXT HEIGHT: %i\n", context.height);

	int c;
	for (c=0; c<context.count; c++)
	{
		//Silly DMA stuff
		for (i=0; i<3; i++)
		{
			DMA[i].ptr = 0;
			DMA[i].dst = 0;
		}
		
		switch (context.cmd_next)
		{
		case CMD_PRESSURE:
			DMA[0].ptr = context.pressure;
			DMA[0].dst = context.pressure;
			DMA[0].buffLen = context.width*4;
			
			DMA[1].ptr = context.velocityX;
			DMA[1].buffLen = context.width*4;
			
			DMA[2].ptr = context.velocityY;
			DMA[2].buffLen = context.width*4;
			break;
			
		case CMD_PRESSURE_APPLY:
			break;
			
		case CMD_VISCOSITY:
			DMA[0].ptr = context.velocityX;
			DMA[0].dst = context.velocityX;
			DMA[0].buffLen = context.width*4;
			
			DMA[1].ptr = context.velocityY;
			DMA[1].dst = context.velocityY;
			DMA[1].buffLen = context.width*4;
			break;
		}
		
		//Prepare the DMA streams... (next frame!)
		int left = 0;
		int right = MAX-1;
		
		if (left + context.start < 0)
		{
			left = left - context.start;
		}
		
		if (right + context.start >= context.height)
		{
			right = context.height - context.start - 1;
		}
		
		int dmaSize = right-left+1;
		while (dmaSize > 0)
		{
			int dmaDo = 32;
			if (dmaSize > dmaDo)
			{
				dmaSize -= dmaDo;
			}
			else
			{
				dmaSize = 0;
			}
			
			dmaLoad(left, left+context.start,dmaSize);
			
			left += dmaSize;
		}
		
		//Done?
		if (context.cmd == CMD_NOOP)
			return 0;
		
		//Remap our data...  (what we loaded to work on previously is what we'll use
		//now)
		onoff = 1-onoff;
		if (onoff)
		{
			for (i=0; i<FLUID_BUFFERS; i++)
				dat[i].d = realData[i].d;
		}
		else
		{
			for (i=0; i<FLUID_BUFFERS; i++)
				dat[i].d = realData[(i+6)%MAX].d;
		}

		//Now, let us start...
		if (context.reverse)
			reverse();
		else
			forward();
		
		context.start += MAX-1;
		
		if (context.start >= context.height)
			return 0;
		
		dmaComplete();
	}

	return 0;
}
