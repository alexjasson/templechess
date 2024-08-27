#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include "Branch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

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
    buffer[strcspn(buffer, "\n")] = 0;

    char *ptr = buffer;
    while (*ptr != '\0' && isspace((unsigned char)*ptr)) ptr++;
    if (*ptr == '\0') continue;

    char *lastSpace = strrchr(buffer, ' ');

    sscanf(lastSpace, " %ld", &nodes);
    *lastSpace = '\0';

    lastSpace = strrchr(buffer, ' ');
    sscanf(lastSpace, " %d", &depth);
    *lastSpace = '\0';

    fen = buffer;

    // Save the current stdout
    fflush(stdout); // Ensure all output is flushed before redirection
    int savedStdout = dup(STDOUT_FILENO);
    int devNull = open("/dev/null", O_WRONLY);
    if (devNull == -1) {
      perror("open");
      close(savedStdout);
      return 1;
    }

    // Redirect stdout to /dev/null
    if (dup2(devNull, STDOUT_FILENO) == -1) {
      perror("dup2");
      close(savedStdout);
      close(devNull);
      return 1;
    }
    close(devNull);

    // Call ChessBoardTreeSearch and suppress output
    ChessBoard cb = ChessBoardNew(fen, depth);
    long result = BranchTreeSearch(&cb);

    // Restore the original stdout
    fflush(stdout); // Ensure all output is flushed to /dev/null
    if (dup2(savedStdout, STDOUT_FILENO) == -1) {
      perror("dup2");
      close(savedStdout);
      return 1;
    }
    close(savedStdout);

    // Print results after restoring stdout
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



