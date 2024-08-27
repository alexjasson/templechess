#ifndef BRANCH_H
#define BRANCH_H

#define BRANCH_SIZE 20

typedef struct {
  BitBoard to[BRANCH_SIZE];
  BitBoard from[BRANCH_SIZE];
  int size;
} Branch;

Branch BranchNew();
void BranchAdd(Branch *b, BitBoard to, BitBoard from);
int BranchIsEmpty(Branch *b, int index);
long BranchTreeSearch(ChessBoard *cb);

#endif