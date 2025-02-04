#ifndef MOVESET_H
#define MOVESET_H

#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"

#define MAPS_SIZE 32 // Assumes only regular chess positions will be given

/*
 * Represents a mapping between a set of from squares and a set
 * of to squares for a given piece. It implicity stores the legal moves
 * for that piece. Can be injective, bijective or surjective mapping.
 */
typedef struct
{
  BitBoard to;
  BitBoard from;
  Piece moved;
} Map;

/*
 * Representation of a set of moves
 */
typedef struct
{
  Map maps[MAPS_SIZE];
  int size;  // Number of maps
  Move prev; // The previous move that was removed from the set
} MoveSet;

/*
 * Returns a new empty set of moves
 */
MoveSet MoveSetNew(); // Stack allocated

/*
 * Given a chess board, fill the given set of moves with all the legal moves.
 * Assumes that the moveset is empty.
 */
void MoveSetFill(LookupTable l, ChessBoard *cb, MoveSet *ms);

/*
 * Given a set of moves, return the size of the set
 */
int MoveSetCount(MoveSet *ms);

/*
 * Given a set of moves, remove the last move from the set and returns it.
 * Assumes that the moveset is not empty.
 */
Move MoveSetPop(MoveSet *ms);

/*
 * Given a set of moves, return whether the set is empty
 */
int MoveSetIsEmpty(MoveSet *ms);

#endif
