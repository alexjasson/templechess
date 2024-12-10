#ifndef BRANCH_H
#define BRANCH_H

#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"

#define BRANCH_SIZE 20 // Assumes only regular chess positions will be given

typedef struct
{
  BitBoard to;
  BitBoard from;
  Piece moved;
} Branch;

Branch BranchNew(BitBoard to, BitBoard from, Piece moved);
int BranchCount(Branch *b, int size);
int BranchExtract(Branch *b, int size, Move *moves);
int BranchFill(LookupTable l, ChessBoard *cb, Branch *b);
int BranchPrune(LookupTable l, ChessBoard *cb, Branch *b, int size);

#endif
