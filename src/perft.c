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

static long treeSearch(LookupTable l, ChessBoard *cb, int depth, int base);

int main(int argc, char **argv)
{
  // Check arguments
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <fen> <depth>\n", argv[0]);
    return 1;
  }

  LookupTable l = LookupTableNew();
  ChessBoard cb = ChessBoardNew(argv[1]);
  int depth = atoi(argv[2]);
  long nodes = treeSearch(l, &cb, depth, 1);
  printf("\nNodes searched: %ld\n", nodes);
  LookupTableFree(l);
  return 0;
}

static long treeSearch(LookupTable l, ChessBoard *cb, int depth, int base)
{
  BranchSet bs = BranchSetNew();
  BranchFill(l, cb, &bs);

  if (depth == 0)
    return 1;

  if ((depth == 1) && (!base))
    return BranchCount(&bs);

  long nodes = 0;
  while (!BranchIsEmpty(&bs)) {
    Move m = BranchPop(&bs);
    ChessBoard newBoard = ChessBoardPlayMove(cb, m);
    long subTree = treeSearch(l, &newBoard, depth - 1, 0);
    if (base)
      ChessBoardPrintMove(m, subTree);
    nodes += subTree;
  }

  return nodes;
}
