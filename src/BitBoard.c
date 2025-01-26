#include <stdio.h>
#include <stdlib.h>

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

BitBoard BitBoardAdd(BitBoard b, Square s)
{
  return b | ((BitBoard)1 << s);
}

int BitBoardCount(BitBoard b)
{
  return __builtin_popcountll(b);
}

Square BitBoardPeek(BitBoard b)
{
  return __builtin_ctzll(b);
}

Square BitBoardPop(BitBoard *b)
{
  Square s = BitBoardPeek(*b);
  *b &= *b - 1;
  return s;
}

// Note: a8 is rank 0, a1 is rank 7
int BitBoardRank(Square s)
{
  return s >> 0x3;
}

// Note: a8 is file 0, h8 is file 7
int BitBoardFile(Square s)
{
  return s & 0x7;
}

int BitBoardDiagonal(Square s)
{
  return BitBoardRank(s) + BitBoardFile(s);
}

int BitBoardAntiDiagonal(Square s)
{
  return (EDGE_SIZE - 1) + BitBoardRank(s) - BitBoardFile(s);
}

BitBoard BitBoardShiftNE(BitBoard b)
{
  return (b & ~EAST_EDGE) >> 7;
}

BitBoard BitBoardShiftNW(BitBoard b)
{
  return (b & ~WEST_EDGE) >> 9;
}

BitBoard BitBoardShiftN(BitBoard b)
{
  return b >> EDGE_SIZE;
}

BitBoard BitBoardShiftS(BitBoard b)
{
  return b << EDGE_SIZE;
}

BitBoard BitBoardShiftSE(BitBoard b)
{
  return (b & ~EAST_EDGE) << 9;
}

BitBoard BitBoardShiftSW(BitBoard b)
{
  return (b & ~WEST_EDGE) << 7;
}