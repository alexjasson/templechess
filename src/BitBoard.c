#include <stdio.h>
#include <stdlib.h>

#include "BitBoard.h"

void BitBoardPrint(BitBoard b) {
  for (int rank = 0; rank < EDGE_SIZE; rank++) {
    for (int file = 0; file < EDGE_SIZE; file++) {
      int square = (EDGE_SIZE - 1 - rank) * EDGE_SIZE + file;
      printf("%ld ", (b >> square) & 1);
    }
    printf("%d \n", EDGE_SIZE - rank);
  }
  printf("a b c d e f g h\n");
}

BitBoard BitBoardSetBit(BitBoard b, Square s) {
  return b | ((BitBoard)1 << s);
}

// BitBoard BitBoardShiftBit(Square s, Direction d, Magnitude m) {
//   if (m >= EDGE_SIZE) return EMPTY_BOARD;

//   int rank = s / EDGE_SIZE;
//   int file = s % EDGE_SIZE;

//   if (d == North) return (b << (EDGE_SIZE * m));
//   if (d == Northeast) return (b << (EDGE_SIZE * m + m)) & (~FILE_H << m);
//   if (d == East) return (b << m) & ~(FILE_A << m);
//   if (d == Southeast) return (b >> (EDGE_SIZE * m - m)) & (~FILE_H << m);
//   if (d == South) return (b >> (EDGE_SIZE * m));
//   if (d == Southwest) return (b >> (EDGE_SIZE * m + m)) & (~FILE_A >> m);
//   if (d == West) return (b >> m) & (~FILE_H >> m);
//   if (d == Northwest) return (b << (EDGE_SIZE * m - m)) & (~FILE_A >> m);

//   fprintf(stderr, "Invalid direction!\n");
//   exit(EXIT_FAILURE);
// }
