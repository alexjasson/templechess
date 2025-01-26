/*
 * TempleChess v0
 * © 2025 Alex Jasson
 */

#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include "MoveSet.h"
#include <stdio.h>
#include <stdlib.h>

static long treeSearch(LookupTable l, ChessBoard *cb, int depth, int base);

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

static long treeSearch(LookupTable l, ChessBoard *cb, int depth, int base)
{
  MoveSet ms = MoveSetNew();
  MoveSetFill(l, cb, &ms);

  if (depth == 0)
    return 1;

  if ((depth == 1) && (!base))
    return MoveSetCount(&ms);

  long nodes = 0;
  while (!MoveSetIsEmpty(&ms))
  {
    Move m = MoveSetPop(&ms);
    ChessBoard new = ChessBoardPlayMove(cb, m);
    long subTree = treeSearch(l, &new, depth - 1, 0);
    if (base)
      ChessBoardPrintMove(m, subTree);
    nodes += subTree;
  }

  return nodes;
}
