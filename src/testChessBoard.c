#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define POSITIONS "data/testPositions.in"
#define BUFFER_SIZE 128

int main() {
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
    long result = ChessBoardTreeSearch(cb, FALSE);
    if (result == nodes) {
      printf("\033[0;32mTest PASSED: %s at depth %d\033[0m\n", fen, depth);
    } else {
      printf("\033[0;31mTest FAILED: %s at depth %d\033[0m\n", fen, depth);
      printf("Expected: %ld, got: %ld\n", nodes, result);
    }
  }
  fclose(file);
  return 0;
}