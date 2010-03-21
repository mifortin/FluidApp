/*
 *  bitstream.c
 *  FluidApp
 */

#include "bitstream.h"


struct BitStream
{
	unsigned char *r_dat;
	int numByte;
	int numBit;
	
	unsigned char rot;
};

static void bitStreamFree(void *ptr)
{
	BitStream *bs = (BitStream*)ptr;
	
	if (bs->r_dat)		free(bs->r_dat);
}

BitStream *bitStreamCreate(int in_maxSize)
{
	BitStream *r = x_malloc(sizeof(BitStream), bitStreamFree);
	
	r->r_dat = malloc(in_maxSize);
	
	r->numByte = in_maxSize;
	r->numBit = 0;
	
	r->rot = 1;
	
	return r;
}

void bitStreamReset(BitStream *bs)
{
}

void bitStreamPush(BitStream *bs, int val, int bits)
{
}

int bitStreamRead(BitStream*bs, int bits)
{
}
