#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdint.h>

#define EDGE_SIZE 8
#define BOARD_SIZE 64
#define EMPTY_BOARD 0
#define EMPTY_SQUARE 64

// Bitboards representing the edge squares
#define NORTH_EDGE 0x00000000000000FF
#define EAST_EDGE 0x8080808080808080
#define SOUTH_EDGE 0xFF00000000000000
#define WEST_EDGE 0x0101010101010101

// Bitboards used for castling
#define KINGSIDE_CASTLING 0x9000000000000090
#define QUEENSIDE_CASTLING 0x1100000000000011
#define QUEENSIDE 0x1F1F1F1F1F1F1F1F
#define KINGSIDE 0xF0F0F0F0F0F0F0F0
#define ATTACK_MASK 0x6c0000000000006c
#define OCCUPANCY_MASK 0x6e0000000000006e

/*
 * A square is a number from 0 to 63 representing a square on a chess board.
 * The squares are numbered from a8 to h1.
 */
typedef uint8_t Square;

/*
 * A bitboard is a 64 bit integer representing a set of squares on a chess board.
 */
typedef uint64_t BitBoard;

/*
 * Print a bitboard to stdout
 */
void BitBoardPrint(BitBoard b);

/*
 * Add a square to a bitboard and return the new bitboard, if the square was already present,
 * it won't make a difference.
 */
BitBoard BitBoardAdd(BitBoard b, Square s);

/*
 * If we think of a bitboard as a set of squares, this will return the cardinality of the set
 */
int BitBoardCount(BitBoard b);

/*
 * Given a bitboard, returns the least significant square.
 */
Square BitBoardPeek(BitBoard b);

/*
 * Given a bitboard, returns the least significant square and removes it from the set.
 */
Square BitBoardPop(BitBoard *b);

/*
 * Given a square, returns what rank/file/diagonal/anti diagional it is on. Note that a8 is rank 0,
 * a1 is rank 7 and a8 is file 0, h8 is file 7
 */
int BitBoardRank(Square s);
int BitBoardFile(Square s);
int BitBoardDiagonal(Square s);
int BitBoardAntiDiagonal(Square s);

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