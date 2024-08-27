#include <string.h>

#include "BitBoard.h"
#include "Branch.h"

Branch BranchNew() {
  Branch b;
  memset(&b, 0, sizeof(Branch));
  return b;
}

void BranchAdd(Branch *b, BitBoard to, BitBoard from) {
  b->to[b->size] = to;
  b->from[b->size] = from;
  b->size++;
}

// Returns whether an element in a branch is empty
int BranchIsEmpty(Branch *b, int index) {
  return b->to[index] == EMPTY_BOARD || b->from[index] == EMPTY_BOARD;
}