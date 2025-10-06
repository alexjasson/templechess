/*
 * TempleChess v2
 * © 2025 Alex Jasson
 */

#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include "MoveSet.h"
#include <stdio.h>
#include <stdlib.h>

static long treeSearch(LookupTable l, ChessBoard *cb, int depth);
static long root(LookupTable l, ChessBoard *cb, int depth);

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
  long nodes = root(l, &cb, depth);
  printf("\nNodes searched: %ld\n", nodes);
  LookupTableFree(l);
  return 0;
}

// Base-level function: prints moves and calls treeSearch for each move
static long root(LookupTable l, ChessBoard *cb, int depth)
{
  if (depth == 0)
    return 1;

  MoveSet ms = MoveSetNew();
  MoveSetFill(l, cb, &ms);
  long nodes = 0;

  while (!MoveSetIsEmpty(&ms))
  {
    Move m = MoveSetPop(&ms);
    ChessBoardPlayMove(cb, m);
    long subTree = (depth > 1) ? treeSearch(l, cb, depth - 1) : 1;
    ChessBoardPrintMove(m);
    printf(": %ld\n", subTree);
    nodes += subTree;
    ChessBoardUndoMove(cb, m);
  }

  return nodes;
}

// Recursively search the tree (for non-root levels)
static long treeSearch(LookupTable l, ChessBoard *cb, int depth)
{
  if (depth == 1) 
    return ChessBoardCount(l, cb);
  
  MoveSet ms = MoveSetNew();
  MoveSetFill(l, cb, &ms);

  long nodes = 0;
  if (depth == 2)
    nodes += MoveSetMultiply(l, &ms);

  while (!MoveSetIsEmpty(&ms))
  {
    Move m = MoveSetPop(&ms);
    ChessBoardPlayMove(cb, m);
    nodes += treeSearch(l, cb, depth - 1);
    ChessBoardUndoMove(cb, m);
  }

  return nodes;
}
