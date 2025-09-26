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

// BitBoard representing the rank of a specific square
#define RANK_OF(s) (SOUTH_EDGE >> (EDGE_SIZE * (EDGE_SIZE - BitBoardRank(s) - 1)))

// Bitboards representing the ranks from the perspective of the given color c
#define ENPASSANT_RANK(c) (BitBoard) SOUTH_EDGE >> (EDGE_SIZE * ((c * 3) + 2))
#define PROMOTION_RANK(c) (BitBoard) SOUTH_EDGE >> (EDGE_SIZE * ((!c * 5) + 1))
#define BACK_RANK(c) (BitBoard)((c == White) ? SOUTH_EDGE : NORTH_EDGE)

// Returns a bitboard representing a set of moves given a set of pawns and a color
#define PAWN_ATTACKS(b, c) ((c == White) ? BitBoardShiftNW(b) | BitBoardShiftNE(b) : BitBoardShiftSW(b) | BitBoardShiftSE(b))
#define PAWN_ATTACKS_LEFT(b, c) ((c == White) ? BitBoardShiftNW(b) : BitBoardShiftSE(b))
#define PAWN_ATTACKS_RIGHT(b, c) ((c == White) ? BitBoardShiftNE(b) : BitBoardShiftSW(b))
#define SINGLE_PUSH(b, c) ((c == White) ? BitBoardShiftN(b) : BitBoardShiftS(b))
#define DOUBLE_PUSH(b, c) ((c == White) ? BitBoardShiftN(BitBoardShiftN(b)) : BitBoardShiftS(BitBoardShiftS(b)))

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
 * Bitboard operations
 */
static inline BitBoard BitBoardAdd(BitBoard b, Square s)   { return b | ((BitBoard)1 << s); }
static inline int      BitBoardCount(BitBoard b)           { return __builtin_popcountll(b); }
static inline Square   BitBoardPeek(BitBoard b)            { return __builtin_ctzll(b); }
static inline Square   BitBoardPop(BitBoard *b)            { Square s = BitBoardPeek(*b); *b &= *b - 1; return s; }
static inline int      BitBoardRank(Square s)              { return s >> 3; }
static inline int      BitBoardFile(Square s)              { return s & 7; }
static inline int      BitBoardDiagonal(Square s)          { return BitBoardRank(s) + BitBoardFile(s); }
static inline int      BitBoardAntiDiagonal(Square s)      { return (EDGE_SIZE - 1) + BitBoardRank(s) - BitBoardFile(s); }
static inline BitBoard BitBoardShiftNE(BitBoard b)         { return (b & ~EAST_EDGE) >> 7; }
static inline BitBoard BitBoardShiftNW(BitBoard b)         { return (b & ~WEST_EDGE) >> 9; }
static inline BitBoard BitBoardShiftN(BitBoard b)          { return b >> EDGE_SIZE; }
static inline BitBoard BitBoardShiftS(BitBoard b)          { return b << EDGE_SIZE; }
static inline BitBoard BitBoardShiftSE(BitBoard b)         { return (b & ~EAST_EDGE) << 9; }
static inline BitBoard BitBoardShiftSW(BitBoard b)         { return (b & ~WEST_EDGE) << 7; }

#endif