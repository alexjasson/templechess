#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdint.h>

#define EDGE_SIZE 8
#define BOARD_SIZE 64
#define EMPTY_BOARD 0

typedef uint64_t BitBoard;
typedef uint8_t Bit;

typedef enum {
  a8, b8, c8, d8, e8, f8, g8, h8,
  a7, b7, c7, d7, e7, f7, g7, h7,
  a6, b6, c6, d6, e6, f6, g6, h6,
  a5, b5, c5, d5, e5, f5, g5, h5,
  a4, b4, c4, d4, e4, f4, g4, h4,
  a3, b3, c3, d3, e3, f3, g3, h3,
  a2, b2, c2, d2, e2, f2, g2, h2,
  a1, b1, c1, d1, e1, f1, g1, h1
} Square;

Bit BitBoardGetBit(BitBoard b, Square s);
void BitBoardPrint(BitBoard b);
BitBoard BitBoardSetBit(BitBoard b, Square s);
BitBoard BitBoardPopBit(BitBoard b, Square s);
int BitBoardCountBits(BitBoard b);
Square BitBoardLeastSignificantBit(BitBoard b);
int BitBoardGetRank(Square s);
int BitBoardGetFile(Square s);

// Probably not needed
BitBoard BitBoardShiftNorth(BitBoard b, int magnitude);
BitBoard BitBoardShiftEast(BitBoard b, int magnitude);
BitBoard BitBoardShiftSouth(BitBoard b, int magnitude);
BitBoard BitBoardShiftWest(BitBoard b, int magnitude);

#endif