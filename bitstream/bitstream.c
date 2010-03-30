/*
 *  bitstream.c
 *  FluidApp
 */

#include "bitstream.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

struct BitStream
{
	unsigned char *r_dat;
	int numByte;
	int numBit;
	
	int curBit;
	
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
	memset(r->r_dat, 0, in_maxSize);
	
	r->numByte = in_maxSize;
	r->numBit = 0;
	r->curBit = 0;
	
	r->rot = 1;
	
	return r;
}

void bitStreamReset(BitStream *bs)
{
	bs->rot = 1;
	bs->numBit = 0;
	bs->curBit = 0;
}

void bitStreamClear(BitStream *bs)
{
	bitStreamReset(bs);
	memset(bs->r_dat, 0, bs->numByte);
}

int bitStreamSize(BitStream *bs)
{
	return bs->numBit;
}

static int bitMask[32] =
{	0x0,
	0x1,		0x3,		0x7,		0xF,
	0x1F,		0x3F,		0x7F,		0xFF,
	0x1FF,		0x3FF,		0x7FF,		0xFFF,
	0x1FFF,		0x3FFF,		0x7FFF,		0xFFFF,
	
	0x1FFFF,	0x3FFFF,	0x7FFFF,	0xFFFFF,
	0x1FFFFF,	0x3FFFFF,	0x7FFFFF,	0xFFFFFF,
	0x1FFFFFF,	0x3FFFFFF,	0x7FFFFFF,	0xFFFFFFF,
	0x1FFFFFFF,	0x3FFFFFFF,	0x7FFFFFFF};

void bitStreamPush(BitStream *bs, int val, int bits)
{
	unsigned int tp = (val & bitMask[bits]) << (sizeof(int)*8 - bits);
	
	unsigned int L = tp >> (bs->curBit);
	unsigned int R = tp << (32-bs->curBit);
	
	unsigned int *d = ((unsigned int*)bs->r_dat + bs->numBit);

	d[0] |= L;
	
	bs->curBit += bits;
	if (bs->curBit >= 32)
	{
		d[1] |= R;
		bs->curBit-=32;
		bs->numBit++;
	}
}

int bitStreamRead(BitStream *bs, int bits)
{
	unsigned int *d = (unsigned int*)(bs->r_dat + bs->numBit);
	
	int toRet = ((d[0] << (bs->curBit))
						>> (sizeof(unsigned int)*8-bits))
							& bitMask[bits];
	
	bs->curBit += bits;
	while (bs->curBit >= 8)
	{
		bs->curBit-=8;
		bs->numBit++;
	}
	
	return toRet;
}


void bitStreamEncodeField(BitStream *bs, field *f, void *buff, int r)
{
	if (fieldIsCharData(f))
	{
		int offY = fieldStrideY(f)*r;
		
		short *b = (short*)buff;
		unsigned char *d = fieldCharData(f)+offY;
		
		int x;
		int w = fieldWidth(f);
		int nc = fieldComponents(f);
		short *amt = b+w;
		int c;
		for (c=0; c<nc; c++)
		{
			b[0] = d[0+c];
			for (x=1; x<w; x++)
			{
				b[x] = d[c+(x-1)*nc]-d[c+x*nc];
				
				if (b[x] & 0x100)
				{
					b[x] = -b[x];
					b[x] |= 0x100;
				}
			}
		
			int mask = 1;
			int rot = 0;
			int bit;
			for (bit=0; bit < 9; bit++)
			{
				int count = 0;
				int total = 1;
				int maxCount = 0;
				int ohBits = 1;
				int oh = 1;
				int prev = b[0] & mask;
				//Pass 1 - # of bits...
				for (x=0; x<w && w > total*ohBits+1; x++)
				{
					if (prev == (b[x] & mask))
						count++;
					else
					{
						if (count > maxCount)
						{
							maxCount = count;
							while (maxCount > oh+1)
							{
								oh = oh << 1;
								ohBits++;
							}
						}
						
						amt[total-1] = count;
						
						total++;
						
						prev = b[x] & mask;
						count = 1;
					}
				}
				amt[total-1] = count;
				
				if (count > maxCount)
				{
					maxCount = count;
				
					while (maxCount > oh+1)
					{
						oh = oh << 1;
						ohBits++;
					}
				}
				
				if (w <= total * ohBits+1)
				{
					bitStreamPush(bs, 0, 4);
					
					//Write out the bits, as is...
					for (x=0; x<w; x++)
					{
						bitStreamPush(bs, (b[x]&mask)>>rot, 1);
					}
				}
				else
				{
					bitStreamPush(bs, ohBits, 4);
					
					prev = b[0] & mask;
					bitStreamPush(bs, prev>>rot, 1);	//start bit...
					
					count = 0;
					//total = 0;
					
					//Pass 2 - write!
					for (x=0; x<total; x++)
					{
						bitStreamPush(bs, amt[x]-1, ohBits);
					}
				}
				
				mask = mask << 1;
				rot ++;
			}
		}
	}
	
}


void bitStreamDecodeField(BitStream *bs, field *f, void *buff, int r)
{
	if (fieldIsCharData(f))
	{
		int offY = fieldStrideY(f)*r;
		
		short *b = (short*)buff;
		unsigned char *d = fieldCharData(f)+offY;
		
		int x;
		int w = fieldWidth(f);
		int nc = fieldComponents(f);
		int c;
		
		for (c=0; c<nc; c++)
		{
			int mask = 1;
			int bit;
			for (x=0; x<w; x++)
			{
				b[x] = 0;
			}
			
			for (bit=0; bit < 9; bit++)
			{
				int ohBits = bitStreamRead(bs, 4);
				if (ohBits == 0)
				{
					for (x=0; x<w; x++)
					{
						b[x] |= mask * bitStreamRead(bs, 1);
					}
				}
				else
				{
					int cb = bitStreamRead(bs, 1);
					x=0;
					while (x < w)
					{
						int size = bitStreamRead(bs, ohBits) + 1;
						int i;
						for (i=0;i<size; i++)
						{
							b[x] |= mask*cb;
							x++;
						}
						
						cb = 1-cb;
					}
				}
				
				mask = mask << 1;
			}
			
			d[c] = b[0];
			for (x=1; x<w; x++)
			{
				if (b[x] & 0x100)
				{
					b[x] &= 0xFF;
					b[x] = -b[x];
				}
				
				d[c+x*nc] = d[c+(x-1)*nc]-b[x];
			}
		}
	}
}

#define M	16
#define R	5

void bitStreamEncodeFelics(BitStream *bs, field *f, void *buff, int r)
{
	if (!fieldIsCharData(f))
		return;
	
	short *b = (short*)buff;
	int w = fieldWidth(f);
	int nc = fieldComponents(f);
	int offY = fieldStrideY(f)*r;
	unsigned char *d = fieldCharData(f)+offY;
	
	if (r == 0)
	{
		int x,c;
		
		for (c=0; c<nc; c++)
		{
			b[c] = d[c];
			
			bitStreamPush(bs, 0xFFFFFF, b[c]/M);
			
			bitStreamPush(bs, b[c] - M * (b[c]/M), R);
		}
		
		for (x=nc; x<w*nc; x++)
		{
			b[x] = d[x-nc] - d[x];
			
			b[x] *= 2;
			if (b[x] < 0)
				b[x] = -b[x]-1;
		
			bitStreamPush(bs, 0xFFFFFF, b[x]/M);
			
			bitStreamPush(bs, b[x] - M * (b[x]/M), R);
		}
	}
	else
	{
		int x,c;
		
		for (c=0; c<nc; c++)
		{
			b[c] = d[c];
			
			bitStreamPush(bs, 0xFFFFFF, b[c]/M);
			
			bitStreamPush(bs, b[c] - M * (b[c]/M), R);
		}
		
		for (x=nc; x<w*nc; x++)
		{
			int up = b[x];
			int left = b[x-nc];
			b[x] = d[x-nc] - d[x];
			
			b[x] = b[x] << 1;
			if (b[x] < 0)
				b[x] = -b[x]-1;
				
			int cur = b[x];
			
			if (left < up)
			{
				int a = left;
				left = up;
				up = a;
			}
			
			int d = left - up;
			
			int numBits = 32 - __builtin_clz(d);		// # of leading 0 bits...
			int mask = ~(0xFFFFFFFF << numBits);
			
			int mid = (left+up) >> 1;		//Div 2
			int start = mid - (mask >> 1);
			
			if (left == up)
			{
				start = left;
				mask = 0;
				numBits = 0;
			}
			
			if (cur >= start && cur <= start+mask && numBits < b[x]/M + R)
			{
				//bitStreamPush(bs, 0, 1);//	- implied in next statement
				
				bitStreamPush(bs, cur - start, numBits+1);	//Adds in a zero
			}
			else
			{
				//bitStreamPush(bs, 1, 1);//	- implied in next statement
		
				bitStreamPush(bs, 0xFFFFFFF, b[x]/M+1);
				
				bitStreamPush(bs, b[x] - M * (b[x]/M), R);
			}
		}
	}
}


void bitStreamDecodeFelics(BitStream *bs, field *f, void *buff, int r)
{
	if (!fieldIsCharData(f))
		return;
	
	short *b = (short*)buff;
	int w = fieldWidth(f);
	int nc = fieldComponents(f);
	int offY = fieldStrideY(f)*r;
	unsigned char *d = fieldCharData(f)+offY;
	
	if (r == 0)
	{
		int x,c;
		
		for (c=0; c<nc; c++)
		{
			int r = 0;
			while (bitStreamRead(bs, 1))
				r++;
			
			b[c] = M*r + bitStreamRead(bs, R-1);
			d[c] = b[c];
		}
		
		for (x=1; x<w; x++)
		{
			for (c=0; c<nc; c++)
			{
				int r = 0;
				while (bitStreamRead(bs, 1))
					r++;
				
				b[x*nc + c] = M*r + bitStreamRead(bs, R-1);
				
				int decode = b[x*nc+c];
			
				if (decode & 0x1)
					decode = -(decode+1)/2;
				else
					decode = decode/2;
				
				d[x*nc+c] = d[(x-1)*nc+c] - decode;
			}
		}
	}
	else
	{
		int x,c;
		
		for (c=0; c<nc; c++)
		{
			int r = 0;
			while (bitStreamRead(bs, 1))
				r++;
			
			b[c] = M*r + bitStreamRead(bs, R-1);
			d[c] = b[c];
		}
		
		for (x=1; x<w; x++)
		{
			for (c=0; c<nc; c++)
			{
				int up = b[x*nc + c];
				int left = b[(x-1)*nc + c];
				
				if (left < up)
				{
					int a = left;
					left = up;
					up = a;
				}
				
				int dist = left - up;
				
				int numBits = 32 - __builtin_clz(dist);		// # of leading 0 bits...
				int mask = ~(0xFFFFFFFF << numBits);
				
				int mid = (left+up)/2;
				int start = mid - mask/2;
				
				if (left == up)
				{
					start = left;
					mask = 0;
					numBits = 0;
				}
				
				if (0==bitStreamRead(bs, 1))
				{
					if (numBits == 0)
						b[x*nc+c] = start;
					else
						b[x*nc+c] = bitStreamRead(bs, numBits) + start;
				
					int decode = b[x*nc+c];
				
					if (decode & 0x1)
						decode = -(decode+1)/2;
					else
						decode = decode/2;
					
					d[x*nc+c] = d[(x-1)*nc+c] - decode;
				}
				else
				{
					int r = 0;
					while (bitStreamRead(bs, 1))
						r++;
					
					b[x*nc + c] = M*r + bitStreamRead(bs, R-1);
					
					int decode = b[x*nc+c];
				
					if (decode & 0x1)
						decode = -(decode+1)/2;
					else
						decode = decode/2;
					
					d[x*nc+c] = d[(x-1)*nc+c] - decode;
				}
			}
		}
	}
}


void bitStreamEncodeFieldHeader(BitStream *bs, field *f)
{
	bitStreamPush(bs, fieldWidth(f), 16);
	bitStreamPush(bs, fieldHeight(f), 16);
	bitStreamPush(bs, fieldComponents(f), 4);
}


void bitStreamDecodeFieldHeader(BitStream *bs, int *w, int *h, int *c)
{
	*w = bitStreamRead(bs, 16);
	*h = bitStreamRead(bs, 16);
	*c = bitStreamRead(bs, 4);
}

