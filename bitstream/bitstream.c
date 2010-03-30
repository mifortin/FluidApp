/*
 *  bitstream.c
 *  FluidApp
 */

#include "bitstream.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "x_simd.h"

struct BitStream
{
	unsigned int *r_dat;
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


static void bitStreamPushOne(BitStream *bs, const int bits)
{
	unsigned int tp = (~(0xFFFFFFFF << bits)) << (sizeof(int)*8 - bits);
	
	unsigned int L = tp >> (bs->curBit);
	unsigned int R = tp << (32-bs->curBit);
	
	unsigned int *d = (bs->r_dat + bs->numBit);

	d[0] |= L;
	
	bs->curBit += bits;
	if (bs->curBit >= 32)
	{
		d[1] |= R;
		bs->curBit-=32;
		bs->numBit = bs->numBit + 1;
	}
}



static void bitStreamPushExact(BitStream *bs, const int val, const int bits)
{
	unsigned int tp = (val) << (sizeof(int)*8 - bits);
	
	unsigned int L = tp >> (bs->curBit);
	unsigned int R = tp << (32-bs->curBit);
	
	unsigned int *d = (bs->r_dat + bs->numBit);

	d[0] |= L;
	
	bs->curBit += bits;
	if (bs->curBit >= 32)
	{
		d[1] |= R;
		bs->curBit-=32;
		bs->numBit = bs->numBit + 1;
	}
}


void bitStreamPush(BitStream *bs, int val, int bits)
{
	unsigned int tp = (val & (~(0xFFFFFFFF << bits))) << (sizeof(int)*8 - bits);
	
	unsigned int L = tp >> (bs->curBit);
	unsigned int R = tp << (32-bs->curBit);
	
	unsigned int *d = (bs->r_dat + bs->numBit);

	d[0] |= L;
	
	bs->curBit += bits;
	if (bs->curBit >= 32)
	{
		d[1] |= R;
		bs->curBit-=32;
		bs->numBit = bs->numBit + 1;
	}
}


int bitStreamRead(BitStream *bs, int bits)
{
	unsigned int *d = ((unsigned int*)bs->r_dat + bs->numBit);
	
	int rbits = bits - (32 - bs->curBit);
	
	int toRet = ((d[0] << (bs->curBit))
						>> (sizeof(unsigned int)*8-bits))
							& (~(0xFFFFFFFF << bits));
	
	bs->curBit += bits;
	if (bs->curBit >= 32)
	{
		if (rbits > 0)
			toRet = (d[1] >> (sizeof(unsigned int)*8-rbits)) | (toRet);
							
		bs->curBit-=32;
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
	
	int w = fieldWidth(f);
	int nc = fieldComponents(f);
	int offY = fieldStrideY(f)*r;
	unsigned char *d = fieldCharData(f)+offY;
	
	const int lamt = w*nc;
	
	short *bp = (short*)buff + r*lamt - lamt;
	short *b = (short*)buff + r*lamt;
	
	if (r == 0)
	{
		int x,c;
		
		for (c=0; c<nc; c++)
		{
			b[c] = d[c];
			
			bitStreamPushOne(bs, b[c]/M);
			
			bitStreamPushExact(bs, b[c] - M * (b[c]/M), R);
		}
		
		
		for (x=nc; x<lamt; x++)
		{
			b[x] = d[x-nc] - d[x];
			
			b[x] *= 2;
			if (b[x] < 0)
				b[x] = -b[x]-1;
		}
		
		for (x=nc; x<lamt; x++)
		{
		
			bitStreamPushOne(bs, b[x]/M);
			
			bitStreamPushExact(bs, b[x] - M * (b[x]/M), R);
		}
	}
	else
	{
		int x,c;
		
		for (c=0; c<nc; c++)
		{
			const int cur = b[c] = d[c];
			
			bitStreamPushOne(bs, cur/M);
			
			bitStreamPushExact(bs, cur - M * (cur/M), R);
		}
		
		
#ifdef __APPLE_ALTIVEC__
		vector short *vb = (vector short*)b;
		vector unsigned char *vd = (vector unsigned char*)d;
		
		for (x=nc; x<16; x++)
		{
			int bx = d[x-nc] - d[x];
			bx = bx * 2;
			if (bx < 0)
				bx = -bx-1;

			b[x] = bx;
		}
		
		vector short n1 = {-1,-1,-1,-1,-1,-1,-1,-1};
		vector short one = {1,1,1,1,1,1,1,1};
		vector short zero = {0,0,0,0,0,0,0,0};
		vector unsigned char pmuteH = {	0x00, 0x10, 0x00, 0x11,
										0x00, 0x12, 0x00, 0x13,
										0x00, 0x14, 0x00, 0x15,
										0x00, 0x16, 0x00, 0x17};
		vector unsigned char pmuteL = {	0x00, 0x18, 0x00, 0x19,
										0x00, 0x1A, 0x00, 0x1B,
										0x00, 0x1C, 0x00, 0x1D,
										0x00, 0x1E, 0x00, 0x1F};
		for (x=1; x<lamt/16; x++)
		{
			vector unsigned char left = vec_sld(vd[x-1], vd[x], 16-3);
			
			vector short bx = vec_sub(vec_perm(zero,left,pmuteH), vec_perm(zero,vd[x],pmuteH));
			bx = vec_sl(bx, one);
			
			vector short mask = vec_cmplt(bx, zero);
			
			vector short mn1 = vec_sub(vec_abs(bx), n1);
			
			vb[x*2+0] = vec_sel(bx, mn1, mask);
			
			bx = vec_sub(vec_perm(zero,left,pmuteL), vec_perm(zero,vd[x],pmuteL));
			bx = vec_sl(bx, one);
			
			mask = vec_cmplt(bx, zero);
			
			mn1 = vec_sub(vec_abs(bx), n1);
			
			vb[x*2+1] = vec_sel(bx, mn1, mask);
		}
#else
		for (x=nc; x<lamt; x++)
		{
			int bx = d[x-nc] - d[x];
			bx = bx * 2;
			if (bx < 0)
				bx = -bx-1;

			b[x] = bx;
		}
#endif
		
		for (x=nc; x<lamt; x++)
		{
			int up = bp[x];
			int left = b[x-nc];
			int bx = b[x];
			
			const int cur = bx;
			
			if (left < up)
			{
				int a = left;
				left = up;
				up = a;
			}
			
			if (left == up)
			{
				if (cur == left)
					bitStreamPushExact(bs, 0, 1);
				else
				{
					bitStreamPushOne(bs, cur/M+1);
					
					bitStreamPushExact(bs, cur - M * (cur/M), R);
				}
			}
			else
			{
				int d;
				d = left - up;
				
				int numBits = 32 - __builtin_clz(d);		// # of leading 0 bits...
				unsigned int mask = 0xFFFFFFFF >> (32-numBits);
				
				int mid = (left+up) /2;		//Div 2
				int start = mid - (mask /2);
			
				int cmStart = cur-start;
				if (cmStart >= 0 && cmStart <= mask && numBits*M < cur + R*M)
				{
					//bitStreamPush(bs, 0, 1);//	- implied in next statement
					
					bitStreamPushExact(bs, cmStart, numBits+1);	//Adds in a zero
				}
				else
				{
					//bitStreamPush(bs, 1, 1);//	- implied in next statement
			
					bitStreamPushOne(bs, cur/M+1);
					
					bitStreamPushExact(bs, cur - M * (cur/M), R);
				}
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
				
				int mask, numBits, start;
				
				if (left == up)
				{
					start = left;
					mask = 0;
					numBits = 0;
				}
				else
				{
					int dist = left - up;
					
					numBits = 32 - __builtin_clz(dist);		// # of leading 0 bits...
					mask = ~(0xFFFFFFFF << numBits);
					
					int mid = (left+up)/2;
					start = mid - mask/2;
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

