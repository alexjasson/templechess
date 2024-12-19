#ifndef LOOKUP_TABLE_H
#define LOOKUP_TABLE_H

typedef struct lookupTable *LookupTable;

// Each type of piece on a chess board
typedef enum
{
  Pawn,
  King,
  Knight,
  Bishop,
  Rook,
  Queen
} Type;

// Each color a piece/player can be
typedef enum
{
  White,
  Black
} Color;

/*
 * Creates a new lookup table, roughly 2MB in size on the heap.
 */
LookupTable LookupTableNew(void);

/*
 * Free the lookup table from memory.
 */
void LookupTableFree(LookupTable l);

/*
 * Given a square, type of piece, and a set of occupancies, return a bitboard
 * representing the squares that the piece could attack.
 */
BitBoard LookupTableAttacks(LookupTable l, Square s, Type t, BitBoard o);

/*
 * Given two squares, return a bitboard representing the squares between them (exclusive).
 */
BitBoard LookupTableGetSquaresBetween(LookupTable l, Square s1, Square s2);

/*
 * Given two squares, returns all the squares of a rank/file/diagonal/antidiagonal they're on,
 * if they're not on the same rank/file/diagonal/antidiagonal, return an empty bitboard.
 */
BitBoard LookupTableGetLineOfSight(LookupTable l, Square s1, Square s2);

#endif