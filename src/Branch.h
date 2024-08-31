#ifndef BRANCH_H
#define BRANCH_H

#define BRANCH_SIZE 20

typedef struct {
  BitBoard to[BRANCH_SIZE];
  BitBoard from[BRANCH_SIZE];
  Piece moved[BRANCH_SIZE];
  int size;
} Branch;

int BranchCount(Branch *b);
int BranchExtract(Branch *b, Move *moves);
Branch BranchNew(LookupTable l, ChessBoard *cb); // Stack allocated

#endif