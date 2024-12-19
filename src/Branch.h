#ifndef BRANCH_H
#define BRANCH_H

#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"

#define BRANCHES_SIZE 20 // Assumes only regular chess positions will be given

/*
 * A branch represents a mapping between a set of from squares and a set
 * of to squares for a given piece. It implicity stores the legal moves
 * for that piece.
 */
typedef struct
{
  BitBoard to;
  BitBoard from;
  Piece moved;
} Branch;

/*
 * Creates a new branch with the given to, from, and moved values
 */
Branch BranchNew(BitBoard to, BitBoard from, Piece moved);

/*
 * Given an array of empty branches and a chess position, fill each branch with
 * the legal moves for each piece on the board. The first branch stores an
 * injective mapping of all the legal king moves. The following branches store
 * an injective mapping of each of the piece moves. There are then 4 branches
 * which stores a bijective mapping of the left pawn attacks, right pawn attacks,
 * single pawn pushes and double pawn pushes. Finally, the last branch stores a
 * surjective mapping of en passant moves.
 */
int BranchFill(LookupTable l, ChessBoard *cb, Branch *b);

/*
 * Given an array of branches and the size of that array, return the toal number
 * of moves in all the branches.
 */
int BranchCount(Branch *b, int size);

/*
 * Given an array of branches and the size of that array, extract all the moves
 * from each branch and store them in the given moves array. Returns the number
 * of moves stored.
 */
int BranchExtract(Branch *b, int size, Move *moves);

#endif
