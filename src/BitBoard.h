#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdint.h>

// Bitboards representing the edge squares
#define NORTH_EDGE 0x00000000000000FF
#define EAST_EDGE 0x8080808080808080
#define SOUTH_EDGE 0xFF00000000000000
#define WEST_EDGE 0x0101010101010101

#define EDGE_SIZE 8
#define BOARD_SIZE 64
#define EMPTY_BOARD 0
#define EMPTY_SQUARE 64

typedef uint64_t BitBoard; // A bitboard is a representation of a set of squares on a chess board
typedef uint8_t Square;    // a8 - h8 = 0 - 7, a1 - h1 = 56 - 63, 64 = Empty Square

/*
 * Print a bitboard to stdout
 */
void BitBoardPrint(BitBoard b);

/*
 * Add a square to a bitboard and return the new bitboard, if the square was already present,
 * it won't make a difference.
 */
BitBoard BitBoardSetBit(BitBoard b, Square s);

/*
 * Remove a square from a bitboard and return the new bitboard, if the square wasn't already
 * present, it won't make a difference.
 */
BitBoard BitBoardPopBit(BitBoard b, Square s);

/*
 * If we think of a bitboard as a set of squares, this will return the cardinality of the set
 */
int BitBoardCountBits(BitBoard b);

/*
 * Given a bitboard, returns the least significant square.
 */
Square BitBoardGetLSB(BitBoard b);

/*
 * Given a bitboard, returns the least significant square and removes it from the set.
 */
Square BitBoardPopLSB(BitBoard *b);

/*
 * Given a square, returns what rank/file/diagonal/anti diagional it is on. Note that a8 is rank 0,
 * a1 is rank 7 and a8 is file 0, h8 is file 7
 */
int BitBoardGetRank(Square s);
int BitBoardGetFile(Square s);
int BitBoardGetDiagonal(Square s);
int BitBoardGetAntiDiagonal(Square s);

/*
 * Shifts all the squares on a bitboard in a specific direction, there is no wrap around so
 * any squares that are pushed out of bounds are gone.
 */
BitBoard BitBoardShiftNE(BitBoard b);
BitBoard BitBoardShiftNW(BitBoard b);
BitBoard BitBoardShiftN(BitBoard b);
BitBoard BitBoardShiftS(BitBoard b);
BitBoard BitBoardShiftSE(BitBoard b);
BitBoard BitBoardShiftSW(BitBoard b);

#endif