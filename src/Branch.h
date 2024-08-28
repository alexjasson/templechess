#ifndef BRANCH_H
#define BRANCH_H

#define BRANCH_SIZE 20

typedef struct {
  BitBoard to[BRANCH_SIZE];
  BitBoard from[BRANCH_SIZE];
  Type moved[BRANCH_SIZE];
  int size;
} Branch;

Branch BranchNew();
void BranchAdd(Branch *b, BitBoard to, BitBoard from, Type moved);
int BranchIsEmpty(Branch *b, int index);
int BranchCount(Branch *b, Color c);
long BranchTreeSearch(ChessBoard *cb);
int BranchExtract(Branch *b, Move *moveSet, Color c);
BitBoard BranchAttacks(LookupTable l, ChessBoard *cb, Branch *b);

#endif