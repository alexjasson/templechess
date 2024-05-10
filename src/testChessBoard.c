#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define POSITIONS "data/positions.in"
#define BUFFER_SIZE 128

int main() {
  LookupTable l = LookupTableNew();
  FILE *file = fopen(POSITIONS, "r");
  if (file == NULL) {
    fprintf(stderr, "Could not open file: %s\n", POSITIONS);
    return 1;
  }

  char buffer[BUFFER_SIZE];
  int depth;
  long nodes;
  char *fen;

  while (fgets(buffer, sizeof(buffer), file)) {
    // Trim any newline character at the end of the line
    buffer[strcspn(buffer, "\n")] = 0;

    // Check if the line is empty or consists only of whitespace
    char *ptr = buffer;
    while (*ptr != '\0' && isspace((unsigned char)*ptr)) ptr++;
    if (*ptr == '\0') continue;

    char *lastSpace = strrchr(buffer, ' ');

    // Read the nodes number from the position of the last space
    sscanf(lastSpace, " %ld", &nodes);
    *lastSpace = '\0';

    // Find the new last space in the now truncated string
    lastSpace = strrchr(buffer, ' ');
    sscanf(lastSpace, " %d", &depth);
    *lastSpace = '\0';

    // Now string contains only the FEN part
    fen = buffer;

    // Test the ChessBoardTreeSearch function
    ChessBoard cb = ChessBoardNew(fen, depth);
    long result = ChessBoardTreeSearch(l, cb, FALSE);
    if (result == nodes) {
      printf("Test PASSED for position: %s at depth %d\n", fen, depth);
    } else {
      printf("Test FAILED for position: %s at depth %d\n", fen, depth);
      printf("Expected: %ld, got: %ld\n", nodes, result);
    }
  }
  fclose(file);
  return 0;
}