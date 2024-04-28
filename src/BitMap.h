#ifndef BITMAP_H
#define BITMAP_H

#include "BitBoard.h"

typedef uint8_t BitRank; // index 0 is back rank of black, 7 is back rank of white

typedef union {
  BitBoard board;
  BitRank rank[EDGE_SIZE];
} BitMap;

#endif