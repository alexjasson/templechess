#include <stdio.h>
#include "BitBoard.h"

void BitBoardPrint(BitBoard b)
{
  for (int rank = 0; rank < EDGE_SIZE; rank++)
  {
    for (int file = 0; file < EDGE_SIZE; file++)
    {
      Square s = rank * EDGE_SIZE + file;
      printf("%lu ", (b >> s) & 1);
    }
    printf("%d \n", EDGE_SIZE - rank);
  }
  printf("a b c d e f g h\n\n");
}