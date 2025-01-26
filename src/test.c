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

static long treeSearch(LookupTable l, ChessBoard *cb, int depth);

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
    LookupTable l = LookupTableNew();
    long result = treeSearch(l, &cb, depth);
    LookupTableFree(l);

    // Print results
    if (result == nodes)
    {
      printf("\033[0;32mTest PASSED: %s at depth %d\033[0m\n", fen, depth);
    }
    else
    {
      printf("\033[0;31mTest FAILED: %s at depth %d\033[0m\n", fen, depth);
      printf("Expected: %ld, got: %ld\n", nodes, result);
    }
  }
  fclose(file);
  return 0;
}

static long treeSearch(LookupTable l, ChessBoard *cb, int depth)
{
  MoveSet ms = MoveSetNew();
  MoveSetFill(l, cb, &ms);

  if ((depth == 1))
    return MoveSetCount(&ms);

  long nodes = 0;
  while (!MoveSetIsEmpty(&ms))
  {
    Move m = MoveSetPop(&ms);
    ChessBoard new = ChessBoardPlayMove(cb, m);
    nodes += treeSearch(l, &new, depth - 1);
  }

  return nodes;
}
