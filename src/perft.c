/*
 * TempleChess v1
 * © 2025 Alex Jasson
 */

#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include "MoveSet.h"
#include <stdio.h>
#include <stdlib.h>

static long treeSearch(LookupTable l, ChessBoard *cb, int depth, int isRoot);

int main(int argc, char **argv)
{
  // Check arguments
  if (argc != 3)
  {
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

// Recursively search the tree.
static long treeSearch(LookupTable l, ChessBoard *cb, int depth, int isRoot)
{
  if (depth == 0)
    return 1;

  MoveSet ms = MoveSetNew();
  MoveSetFill(l, cb, &ms);
  if (depth == 1 && !isRoot)
    return MoveSetCount(&ms);

  long nodes = 0;
  if (depth == 2 && !isRoot)
    nodes += MoveSetMultiply(l, cb, &ms);

  while (!MoveSetIsEmpty(&ms))
  {
    Move m = MoveSetPop(&ms);
    ChessBoard new = ChessBoardPlayMove(cb, m);
    long subTree = treeSearch(l, &new, depth - 1, 0);
    if (isRoot)
    {
      ChessBoardPrintMove(m);
      printf(": %ld\n", subTree);
    }
    nodes += subTree;
  }

  return nodes;
}
