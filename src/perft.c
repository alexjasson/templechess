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
  LookupTable l = LookupTableNew();
  long nodes = treeSearch(l, &cb, 1);
  printf("\nNodes searched: %ld\n", nodes);
  LookupTableFree(l);
}

static long treeSearch(LookupTable l, ChessBoard *cb, int base)
{
  if (cb->depth == 0)
    return 1;

  Branch branches[BRANCHES_SIZE];
  int branchesSize = BranchFill(l, cb, branches);

  if ((cb->depth == 1) && (!base))
    return BranchCount(branches, branchesSize);

  long nodes = 0;
  ChessBoard new;
  Move moves[MOVES_SIZE];

  int movesSize = BranchExtract(branches, branchesSize, moves);
  for (int i = 0; i < movesSize; i++)
  {
    Move m = moves[i];
    ChessBoardPlayMove(&new, cb, m);
    int subTree = treeSearch(l, &new, 0);
    if (base)
      ChessBoardPrintMove(m, subTree);
    nodes += subTree;
  }

  return nodes;
}
