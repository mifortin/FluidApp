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

BitStream *bitStreamCreate(int in_maxSize)
{
	BitStream *r = x_malloc(sizeof(BitStream), bitStreamFree);
	
	r->r_dat = malloc(in_maxSize);
	r->numByte = in_maxSize;
	
	bitStreamClear(r);
	
	return r;
}


int bitStreamSize(BitStream *bs)
{
	return bs->numBit;
}


static void bitStreamPushExact(BitStream *bs, const int val, const int bits)
{
	unsigned int tp = (val) << (sizeof(int)*8 - bits);
	
	bs->r_dat[bs->numBit] += tp >> (bs->curBit);
	
	bs->curBit += bits;
	if (bs->curBit >= 32)
	{
		bs->r_dat[ bs->numBit+1] = tp << (32-(bs->curBit-bits));
		bs->curBit-=32;
		bs->numBit =  bs->numBit + 1;
	}
//	bs->r_dat[ bs->numBit+1] = tp << (32-(bs->curBit-bits));
//	bs->numBit += bs->curBit/32;
//	bs->curBit = bs->curBit%32;
}



static void bitStreamPushOne(BitStream *bs, const int bits)
{
	bitStreamPushExact(bs, (0xFFFFFFFF >> (32-bits)), bits);
}


void bitStreamPush(BitStream *bs, int val, int bits)
{
	bitStreamPushExact(bs, val & (~(0xFFFFFFFF << bits)), bits);
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
		
		x=nc;
		
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
			
			mn1 = vec_sub(vec_sub(zero,bx), n1);
			
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

		x = nc;
#ifdef __APPLE_ALTIVEC__
		for (; x<8; x++)
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
				goto bscontinue_alti;
			}
			else if (left == up)
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
bscontinue_alti:
				d = left - up;
				
				int numBits = __builtin_clz(d);		// # of leading 0 bits...
				unsigned int mask = 0xFFFFFFFF >> (numBits);
				
				int mid = (left+up) /2;		//Div 2
				int start = mid - (mask /2);
			
				int cmStart = cur-start;
				
				numBits = 32 - numBits;
				
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

		//vector short *vb = (vector short*)b;
		vector short *vbp = (vector short*)bp;
		int i = 0;
		for (; x<lamt-7; x+=8)
		{
			i++;
			vector short vbx = vb[i];
			
			short *pbx = (short*)&vbx;
			
			vector short vup = vbp[i];
			
			vector short vleft = vec_sld(vb[i-1], vb[i], 10);	//16 - 6 (HARDCODED)
			
			vector short mask = vec_cmplt(vleft, vup);
			
			vector short vup2 = vec_sel(vup, vleft, mask);
			
			vector short vleft2 = vec_sel(vleft, vup, mask);
			
			vector short vd = vec_sub(vleft2, vup2);
			
			short *pd = (short*)&vd;
			
			vector short numBits = (vector short){	__builtin_clz(pd[0]),
													__builtin_clz(pd[1]),
													__builtin_clz(pd[2]),
													__builtin_clz(pd[3]),
													__builtin_clz(pd[4]),
													__builtin_clz(pd[5]),
													__builtin_clz(pd[6]),
													__builtin_clz(pd[7])};
			
			vector unsigned short bitmask = (vector unsigned short)
									{0xFFFF, 0xFFFF,0xFFFF, 0xFFFF,0xFFFF, 0xFFFF,0xFFFF, 0xFFFF};
									
			bitmask = vec_sr(bitmask, numBits);
			
			vector short mid = vec_sr(vec_add(vleft,vup),(vector short){1,1,1,1,1,1,1,1});
			
			vector short start = vec_sub(mid,vec_sr(bitmask, (vector short){1,1,1,1,1,1,1,1}));
			
			mask = vec_cmpeq(vleft, vup);
			
			start = vec_sel(start, mid, mask);
			
			numBits = vec_sel(numBits, (vector short){32,32,32,32,32,32,32,32}, mask);
			
			bitmask = vec_sel(bitmask, (vector short){0,0,0,0,0,0,0,0}, mask);
			
			vector short vcmStart = vec_sub(vbx, start);
			
			numBits = vec_sub((vector short){32,32,32,32,32,32,32,32}, numBits);
			
			
			// (a >= b && a <= c)
			// !(a < b || a > c)
			vector short cmp1 = vec_cmplt(vcmStart, (vector short){0,0,0,0,0,0,0,0});
			vector short cmp2 = vec_cmpgt(vcmStart, bitmask);
			
			vector short cmp3left = vec_sl(numBits, (vector short){4,4,4,4,4,4,4,4});
			vector short cmp3right = vec_add(vbx,(vector short){R*M,R*M,R*M,R*M,R*M,R*M,R*M,R*M});
			
			vector short cmp3 = vec_cmplt(cmp3left, cmp3right);
			
			cmp1 = vec_and(cmp3,vec_nor(cmp1, cmp2));
			
			short *cmp = (short*)&cmp1;
			short *cmStart = (short*)&vcmStart;
			short *vNumBits = (short*)&numBits;
			
			int j;
			for (j=0; j<8; j++)
			{
				
				if (cmp[j])
				{
					//bitStreamPush(bs, 0, 1);//	- implied in next statement
					
					bitStreamPushExact(bs, cmStart[j], vNumBits[j]+1);	//Adds in a zero
				}
				else
				{
					//bitStreamPush(bs, 1, 1);//	- implied in next statement
			
					bitStreamPushOne(bs, pbx[j]/M+1);
					
					bitStreamPushExact(bs, pbx[j] - M * (pbx[j]/M), R);
				}
			}
#else
		for (; x<lamt-3; x+=4)
		{
			int bx1 = d[x-nc] - d[x];
			int bx2 = d[x-nc+1] - d[x+1];
			int bx3 = d[x-nc+2] - d[x+2];
			int bx4 = d[x-nc+3] - d[x+3];
			
			bx1 = bx1 * 2;
			if (bx1 < 0)
				bx1 = -bx1-1;
			
			bx2 = bx2 * 2;
			if (bx2 < 0)
				bx2 = -bx2-1;
				
			bx3 = bx3 * 2;
			if (bx3 < 0)
				bx3 = -bx3-1;
			
			bx4 = bx4 * 2;
			if (bx4 < 0)
				bx4 = -bx4-1;

			b[x] = bx1;
			b[x+1] = bx2;
			
			b[x+2] = bx3;
			b[x+3] = bx4;
			
			int up1 = bp[x];
			int up2 = bp[x+1];
			
			int left1 = b[x-nc];
			int left2 = b[x-nc+1];
			
			int up3 = bp[x+2];
			int up4 = bp[x+3];
			
			int left3 = b[x-nc+2];
			int left4 = b[x-nc+3];
			
			const int cur1 = bx1;
			const int cur2 = bx2;
			const int cur3 = bx3;
			const int cur4 = bx4;
			
			if (left1 < up1)
			{
				int a = left1;
				left1 = up1;
				up1 = a;
			}
			
			if (left2 < up2)
			{
				int a = left2;
				left2 = up2;
				up2 = a;
			}
			
			if (left3 < up3)
			{
				int a = left3;
				left3 = up3;
				up3 = a;
			}
			
			if (left4 < up4)
			{
				int a = left4;
				left4 = up4;
				up4 = a;
			}
			
			int d1 = left1 - up1;
			int d2 = left2 - up2;
			int d3 = left3 - up3;
			int d4 = left4 - up4;
			
			int numBits1 = __builtin_clz(d1);
			int numBits2 = __builtin_clz(d2);
			int numBits3 = __builtin_clz(d3);
			int numBits4 = __builtin_clz(d4);
			
			unsigned int mask1 = 0xFFFFFFFF >> numBits1;
			unsigned int mask2 = 0xFFFFFFFF >> numBits2;
			unsigned int mask3 = 0xFFFFFFFF >> numBits3;
			unsigned int mask4 = 0xFFFFFFFF >> numBits4;
			
			int mid1 = (left1+up1) /2;
			int mid2 = (left2+up2) /2;
			int mid3 = (left3+up3) /2;
			int mid4 = (left4+up4) /2;
			
			int start1 = mid1 - (mask1 /2);
			int start2 = mid2 - (mask2 /2);
			int start3 = mid3 - (mask3 /2);
			int start4 = mid4 - (mask4 /2);
		
			
			if (left1 == up1)
			{
				numBits1 = 32;
				start1 = mid1;
				mask1 = 0;
			}
			
			if (left2 == up2)
			{
				numBits2 = 32;
				start2 = mid2;
				mask2 = 0;
			}
			
			if (left3 == up3)
			{
				numBits3 = 32;
				start3 = mid3;
				mask3 = 0;
			}
			
			if (left4 == up4)
			{
				numBits4 = 32;
				start4 = mid4;
				mask4 = 0;
			}
			
			
			int cmStart1 = cur1-start1;
			int cmStart2 = cur2-start2;
			int cmStart3 = cur3-start3;
			int cmStart4 = cur4-start4;
			
			numBits1 = 32 - numBits1;
			numBits2 = 32 - numBits2;
			numBits3 = 32 - numBits3;
			numBits4 = 32 - numBits4;
			
			if (cmStart1 >= 0 && cmStart1 <= mask1 && numBits1*M < cur1 + R*M)
			{
				//bitStreamPush(bs, 0, 1);//	- implied in next statement
				
				bitStreamPushExact(bs, cmStart1, numBits1+1);	//Adds in a zero
			}
			else
			{
				//bitStreamPush(bs, 1, 1);//	- implied in next statement
		
				bitStreamPushOne(bs, cur1/M+1);
				
				bitStreamPushExact(bs, cur1 - M * (cur1/M), R);
			}
			
			if (cmStart2 >= 0 && cmStart2 <= mask2 && numBits2*M < cur2 + R*M)
			{
				//bitStreamPush(bs, 0, 1);//	- implied in next statement
				
				bitStreamPushExact(bs, cmStart2, numBits2+1);	//Adds in a zero
			}
			else
			{
				//bitStreamPush(bs, 1, 1);//	- implied in next statement
		
				bitStreamPushOne(bs, cur2/M+1);
				
				bitStreamPushExact(bs, cur2 - M * (cur2/M), R);
			}
			
			if (cmStart3 >= 0 && cmStart3 <= mask3 && numBits3*M < cur3 + R*M)
			{
				//bitStreamPush(bs, 0, 1);//	- implied in next statement
				
				bitStreamPushExact(bs, cmStart3, numBits3+1);	//Adds in a zero
			}
			else
			{
				//bitStreamPush(bs, 1, 1);//	- implied in next statement
		
				bitStreamPushOne(bs, cur3/M+1);
				
				bitStreamPushExact(bs, cur3 - M * (cur3/M), R);
			}
			
			if (cmStart4 >= 0 && cmStart4 <= mask4 && numBits4*M < cur4 + R*M)
			{
				//bitStreamPush(bs, 0, 1);//	- implied in next statement
				
				bitStreamPushExact(bs, cmStart4, numBits4+1);	//Adds in a zero
			}
			else
			{
				//bitStreamPush(bs, 1, 1);//	- implied in next statement
		
				bitStreamPushOne(bs, cur4/M+1);
				
				bitStreamPushExact(bs, cur4 - M * (cur4/M), R);
			}
#endif
			
		}
		
		for (; x<lamt; x++)
		{
			int up = bp[x];
			int left = b[x-nc];
			int bx = d[x-nc] - d[x];
			bx = bx * 2;
			if (bx < 0)
				bx = -bx-1;

			b[x] = bx;
			
			const int cur = bx;
			
			if (left < up)
			{
				int a = left;
				left = up;
				up = a;
				goto bscontinue;
			}
			else if (left == up)
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
bscontinue:
				d = left - up;
				
				int numBits = __builtin_clz(d);		// # of leading 0 bits...
				unsigned int mask = 0xFFFFFFFF >> (numBits);
				
				int mid = (left+up) /2;		//Div 2
				int start = mid - (mask /2);
			
				int cmStart = cur-start;
				
				numBits = 32 - numBits;
				
				int cmp = (cmStart >= 0) && (cmStart <= mask) && (numBits*M < cur + R*M);
				if (cmp)
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

