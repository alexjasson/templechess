#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdint.h>

#define EDGE_SIZE 8
#define BOARD_SIZE 64
#define EMPTY_BOARD 0
#define FILE_A 0x0101010101010101
#define FILE_H 0x8080808080808080
#define RANK_1 0x00000000000000FF
#define RANK_8 0xFF00000000000000

typedef uint64_t BitBoard;

typedef uint8_t Magnitude;

typedef enum {
  North, Northeast, East, Southeast, South, Southwest, West, Northwest
} Direction;

typedef enum {
  a1, b1, c1, d1, e1, f1, g1, h1,
  a2, b2, c2, d2, e2, f2, g2, h2,
  a3, b3, c3, d3, e3, f3, g3, h3,
  a4, b4, c4, d4, e4, f4, g4, h4,
  a5, b5, c5, d5, e5, f5, g5, h5,
  a6, b6, c6, d6, e6, f6, g6, h6,
  a7, b7, c7, d7, e7, f7, g7, h7,
  a8, b8, c8, d8, e8, f8, g8, h8
} Square;

void BitBoardPrint(BitBoard b);
BitBoard BitBoardSetBit(BitBoard b, Square s);
// BitBoard BitBoardShiftBit(BitBoard b, Direction d, Magnitude m);

#endif