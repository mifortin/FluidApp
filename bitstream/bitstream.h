/*
 *  bitstream.h
 *  FluidApp
 */

#ifndef BITSTREAM_H
#define BITSTREAM_H

#include "memory.h"

typedef struct BitStream BitStream;

BitStream *bitStreamCreate(int in_maxSize);
void bitStreamReset(BitStream *bs);
void bitStreamPush(BitStream *bs, int val, int bits);
int bitStreamRead(BitStream*bs, int bits);

#endif
