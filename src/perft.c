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

static long treeSearch(LookupTable l, ChessBoard *cb, int depth, long *multiplied, int isRoot);

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

  long multiplied = 0;
  long nodes = treeSearch(l, &cb, depth, &multiplied, 1);
  printf("\nNodes searched: %ld\n", nodes + multiplied);
  printf("Edges multiplied: %.2f%%\n", (double)multiplied / (nodes + multiplied) * 100);
  LookupTableFree(l);
  return 0;
}

// Recursively search the tree.
// The extra parameter `isRoot` indicates if this is the top-level call.
// When true, we print each move and its subtree count.
static long treeSearch(LookupTable l, ChessBoard *cb, int depth, long *multiplied, int isRoot)
{
  if (depth == 0)
    return 1;

  MoveSet ms = MoveSetNew();
  MoveSetFill(l, cb, &ms);

  // Base cases
  if (depth == 1)
    return MoveSetCount(&ms);
  if (depth == 2)
    *multiplied += MoveSetMultiply(l, cb, &ms);

  long nodes = 0;
  while (!MoveSetIsEmpty(&ms))
  {
    Move m = MoveSetPop(&ms);
    ChessBoard new = ChessBoardPlayMove(cb, m);
    long subTree = treeSearch(l, &new, depth - 1, multiplied, 0);
    if (isRoot)
    {
      ChessBoardPrintMove(m);
      printf(": %ld\n", subTree);
    }
    nodes += subTree;
  }

  return nodes;
}