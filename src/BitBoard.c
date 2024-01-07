#include <stdio.h>
#include <stdlib.h>

#include "BitBoard.h"

void BitBoardPrint(BitBoard b) {
  for (int rank = 0; rank < EDGE_SIZE; rank++) {
    for (int file = 0; file < EDGE_SIZE; file++) {
      Square s = (EDGE_SIZE - 1 - rank) * EDGE_SIZE + file;
      printf("%ld ", (b >> s) & 1);
    }
    printf("%d \n", EDGE_SIZE - rank);
  }
  printf("a b c d e f g h\n\n");
}

BitBoard BitBoardSetBit(BitBoard b, Square s) {
  return b | ((BitBoard)1 << s);
}

BitBoard BitBoardPopBit(BitBoard b, Square s) {
  return b & ~((BitBoard)1 << s);
}

int BitBoardCountBits(BitBoard b) {
  int count = 0;
  while (b) {
    count++;
    b &= b - 1;
  }
  return count;
}

Square BitBoardLeastSignificantBit(BitBoard b) {
  return BitBoardCountBits((b & -b) - 1);
}