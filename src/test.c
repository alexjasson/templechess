#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include "MoveSet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define POSITIONS "data/testPositions.in"
#define BUFFER_SIZE 128
#define NUM_TESTS 3

typedef int (*TestFunction)(LookupTable, ChessBoard *, int, long);

static long treeSearch(LookupTable l, ChessBoard *cb, TestFunction t, int depth);
static int testChessBoardCount(LookupTable l, ChessBoard *cb, int depth, long nodes);
static int testMoveSetCount(LookupTable l, ChessBoard *cb, int depth, long nodes);
static int testMoveSetMultiply(LookupTable l, ChessBoard *cb, int depth, long nodes);

int main()
{
  FILE *file = fopen(POSITIONS, "r");
  if (file == NULL)
  {
    fprintf(stderr, "Could not open file: %s\n", POSITIONS);
    return 1;
  }

  char buffer[BUFFER_SIZE];
  int depth;
  long nodes;
  char *fen;
  LookupTable l = LookupTableNew();

  TestFunction testFns[NUM_TESTS] = {testChessBoardCount, testMoveSetCount, testMoveSetMultiply};
  const char *testNames[NUM_TESTS] = {"ChessBoardCount", "MoveSetCount", "MoveSetMultiply"};

  for (int i = 0; i < NUM_TESTS; i++)
  {
    printf("\n\033[1;34m============== Running Test: %s ==============\033[0m\n", testNames[i]);
    while (fgets(buffer, sizeof(buffer), file))
    {
      buffer[strcspn(buffer, "\n")] = 0;

      char *ptr = buffer;
      while (*ptr != '\0' && isspace((unsigned char)*ptr))
        ptr++;
      if (*ptr == '\0')
        continue;

      char *lastSpace = strrchr(buffer, ' ');

      sscanf(lastSpace, " %ld", &nodes);
      *lastSpace = '\0';

      lastSpace = strrchr(buffer, ' ');
      sscanf(lastSpace, " %d", &depth);
      *lastSpace = '\0';

      fen = buffer;

      ChessBoard cb = ChessBoardNew(fen);

      // Call the appropriate test function
      int result = testFns[i](l, &cb, depth, nodes);

      if (result)
        printf("\033[0;32mTest PASSED: %s at depth %d\033[0m\n", fen, depth);
    }
    rewind(file);
  }
  LookupTableFree(l);
  fclose(file);
  return 0;
}

static long treeSearch(LookupTable l, ChessBoard *cb, TestFunction t, int depth)
{
  long nodes = 0;
  MoveSet ms = MoveSetNew();
  MoveSetFill(l, cb, &ms);

  if (depth == 1) {
    if (t == testMoveSetCount)
      return MoveSetCount(&ms);
    else // t == ChessBoardCount
      return ChessBoardCount(l, cb);
  }

  if ((depth == 2) && (t == testMoveSetMultiply))
    nodes += MoveSetMultiply(l, &ms);

  while (!MoveSetIsEmpty(&ms))
  {
    Move m = MoveSetPop(&ms);
    ChessBoardPlayMove(cb, m);
    nodes += treeSearch(l, cb, testMoveSetCount, depth - 1);
    ChessBoardUndoMove(cb, m);
  }

  return nodes;
}

static int testChessBoardCount(LookupTable l, ChessBoard *cb, int depth, long nodes) {
  long result = treeSearch(l, cb, testChessBoardCount, depth);

  // Print results
  if (result != nodes)
  {
    printf("\033[0;31mTest FAILED: %s at depth %d\033[0m\n", ChessBoardToFEN(cb), depth);
    printf("Expected: %ld, got: %ld\n", nodes, result);
    return 0; // Failure
  }
  else
  {
    return 1; // Success
  }
}

static int testMoveSetCount(LookupTable l, ChessBoard *cb, int depth, long nodes)
{
  long result = treeSearch(l, cb, testMoveSetCount, depth);

  // Print results
  if (result != nodes)
  {
    printf("\033[0;31mTest FAILED: %s at depth %d\033[0m\n", ChessBoardToFEN(cb), depth);
    printf("Expected: %ld, got: %ld\n", nodes, result);
    return 0; // Failure
  }
  else
  {
    return 1; // Success
  }
}

// Traverses the tree and at each node compares MoveSetCount to MoveSetMultiply
// If they're not equal, print the chessboard that failed, assume depth > 1
static int testMoveSetMultiply(LookupTable l, ChessBoard *cb, int depth, long nodes)
{
  (void)nodes;
  if (depth == 1)
    return 1; // Success
  long count = treeSearch(l, cb, testMoveSetCount, 2);
  long multiply = treeSearch(l, cb, testMoveSetMultiply, 2);

  if (count != multiply)
  {
    printf("\033[0;31mTest FAILED: %s at depth %d\033[0m\n", ChessBoardToFEN(cb), depth);
    printf("Expected: %ld, got: %ld\n", count, multiply);
    return 0; // Failure
  }

  MoveSet ms = MoveSetNew();
  MoveSetFill(l, cb, &ms);

  while (!MoveSetIsEmpty(&ms))
  {
    Move m = MoveSetPop(&ms);
    ChessBoardPlayMove(cb, m);
    int ok = testMoveSetMultiply(l, cb, depth - 1, nodes);
    ChessBoardUndoMove(cb, m);
    if (!ok)
      return 0; // Failure
  }

  return 1; // Success
}