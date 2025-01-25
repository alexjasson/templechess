/*
 * TempleChess v0
 * © 2024 Alex Jasson
 */

#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include "Branch.h"
#include <stdio.h>
#include <stdlib.h>

static long treeSearch(LookupTable l, ChessBoard *cb, int base);

int main(int argc, char **argv)
{
  // Check arguments
  if (argc != 3)
  {
    fprintf(stderr, "Usage: %s <fen> <depth>\n", argv[0]);
    exit(1);
  }

  ChessBoard cb = ChessBoardNew(argv[1], atoi(argv[2]));
  // depth = arg2
  LookupTable l = LookupTableNew();
  long nodes = treeSearch(l, &cb, 1);
  printf("\nNodes searched: %ld\n", nodes);
  LookupTableFree(l);
}

static long treeSearch(LookupTable l, ChessBoard *cb, int base)
{
  // Test if a BranchNew function is slow or not - slowness might be compiler optimized
  BranchSet bs;
  BranchFill(l, cb, &bs);

  if (cb->depth == 0)
    return 1;

  if ((cb->depth == 1) && (!base))
    return BranchCount(&bs);

  long nodes = 0;
  ChessBoard new; // Do I need to do this? Can I return in fn below?

  while (!BranchIsEmpty(&bs)) {
    Move m = BranchPop(&bs);
    ChessBoardPlayMove(&new, cb, m);
    int subTree = treeSearch(l, &new, 0);
    if (base)
      ChessBoardPrintMove(m, subTree);
    nodes += subTree;
  }

  return nodes;
}
