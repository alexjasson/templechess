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
#define EMPTY_SQUARE 64

typedef uint64_t BitBoard;
typedef uint8_t Bit;
typedef uint8_t Square; // a8 - h8 = 0 - 7, a1 - h1 = 56 - 63, 64 = Empty Square

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

#endif