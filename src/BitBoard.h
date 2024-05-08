#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdint.h>

#define NORTH_EDGE 0x00000000000000FF
#define EAST_EDGE 0x8080808080808080
#define SOUTH_EDGE 0xFF00000000000000
#define WEST_EDGE 0x0101010101010101

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
Square BitBoardGetLSB(BitBoard b);
Square BitBoardPopLSB(BitBoard *b);

int BitBoardGetRank(Square s);
int BitBoardGetFile(Square s);
int BitBoardGetDiagonal(Square s);
int BitBoardGetAntiDiagonal(Square s);

BitBoard BitBoardShiftNE(BitBoard b);
BitBoard BitBoardShiftNW(BitBoard b);
BitBoard BitBoardShiftN(BitBoard b);
BitBoard BitBoardShiftS(BitBoard b);
BitBoard BitBoardShiftSE(BitBoard b);
BitBoard BitBoardShiftSW(BitBoard b);
BitBoard BitBoardShiftE(BitBoard b);
BitBoard BitBoardShiftW(BitBoard b);

#endif