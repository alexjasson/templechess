#include <stdio.h>

#include "BitBoard.h"

void BitBoardPrint(BitBoard b) {
  for (int rank = EDGE_SIZE; rank > 0; rank--) {
    printf("%d ", rank);
    for (int file = 0; file < EDGE_SIZE; file++) {
      int square = (rank - 1) * EDGE_SIZE + file;
      printf("%ld ", (b >> square) & 1);
    }
    printf("\n");
  }
  printf("  a b c d e f g h\n");
}

BitBoard BitBoardSetBit(BitBoard b, Square s) {
  return b | ((BitBoard)1 << s);
}