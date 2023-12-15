#include <stdio.h>

#include "BitBoard.h"

void BitBoardPrint(BitBoard b) {
  for (int rank = 0; rank < EDGE_SIZE; rank++) {
    printf("%d ", EDGE_SIZE - rank);
    for (int file = 0; file < EDGE_SIZE; file++) {
      int square = (EDGE_SIZE - 1 - rank) * EDGE_SIZE + file;
      printf("%ld ", (b >> square) & 1);
    }
    printf("\n");
  }
  printf("  a b c d e f g h\n");
}

BitBoard BitBoardSetBit(BitBoard b, Square s) {
  return b | ((BitBoard)1 << s);
}