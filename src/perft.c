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

typedef struct
{
  long counted;    // Number of edges counted
  long multiplied; // Number of edges multiplied by next set of edges
} MoveCount;

static MoveCount treeSearch(LookupTable l, ChessBoard *cb, int depth, int base);

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
  MoveCount mc = treeSearch(l, &cb, depth, 1);
  printf("\nNodes searched: %ld\n", mc.counted + mc.multiplied);
  printf("Edges multiplied: %.2f%%\n", (double)mc.multiplied / (mc.counted + mc.multiplied) * 100);
  LookupTableFree(l);
  return 0;
}

static MoveCount treeSearch(LookupTable l, ChessBoard *cb, int depth, int base)
{
  MoveCount mc = {0, 0};

  if (depth == 0)
  {
    mc.counted = 1;
    return mc;
  }

  MoveSet ms = MoveSetNew();
  MoveSetFill(l, cb, &ms);

  if ((depth == 1) && (!base))
  {
    mc.counted = MoveSetCount(&ms);
    return mc;
  }

  if (depth == 2)
    mc.multiplied += MoveSetMultiply(l, cb, &ms);

  while (!MoveSetIsEmpty(&ms))
  {
    Move m = MoveSetPop(&ms);
    ChessBoard new = ChessBoardPlayMove(cb, m);
    MoveCount subTree = treeSearch(l, &new, depth - 1, 0);
    if (base)
      ChessBoardPrintMove(m, subTree.counted + subTree.multiplied);
    mc.counted += subTree.counted;
    mc.multiplied += subTree.multiplied;
  }
  return mc;
}
