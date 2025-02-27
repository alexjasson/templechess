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

static long treeSearch(LookupTable l, ChessBoard *cb, int depth, long *multiplied);
static long root(LookupTable l, ChessBoard *cb, int depth, long *multiplied);

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
  long nodes = root(l, &cb, depth, &multiplied);
  printf("\nNodes searched: %ld\n", nodes);
  printf("Edges multiplied: %.2f%%\n", (double)multiplied / nodes * 100);
  LookupTableFree(l);
  return 0;
}

// Base-level function: prints moves and calls treeSearch for each move
static long root(LookupTable l, ChessBoard *cb, int depth, long *multiplied)
{
  if (depth == 0)
    return 1;

  MoveSet ms = MoveSetNew();
  MoveSetFill(l, cb, &ms);
  long nodes = 0;

  while (!MoveSetIsEmpty(&ms))
  {
    Move m = MoveSetPop(&ms);
    ChessBoard new = ChessBoardPlayMove(cb, m);
    long subTree = (depth > 1) ? treeSearch(l, &new, depth - 1, multiplied) : 1;
    ChessBoardPrintMove(m);
    printf(": %ld\n", subTree);
    nodes += subTree;
  }

  return nodes;
}

// Recursively search the tree (for non-root levels)
static long treeSearch(LookupTable l, ChessBoard *cb, int depth, long *multiplied)
{
  MoveSet ms = MoveSetNew();
  MoveSetFill(l, cb, &ms);

  if (depth == 1)
    return MoveSetCount(&ms);

  long nodes = 0;
  if (depth == 2)
  {
    nodes += MoveSetMultiply(l, cb, &ms);
    *multiplied += nodes;
  }

  while (!MoveSetIsEmpty(&ms))
  {
    Move m = MoveSetPop(&ms);
    ChessBoard new = ChessBoardPlayMove(cb, m);
    nodes += treeSearch(l, &new, depth - 1, multiplied);
  }

  return nodes;
}
