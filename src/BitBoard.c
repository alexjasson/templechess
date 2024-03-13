#include <stdio.h>
#include <stdlib.h>

#include "BitBoard.h"

Bit BitBoardGetBit(BitBoard b, Square s) {
  return (Bit)(b >> s) & 1;
}

void BitBoardPrint(BitBoard b) {
  for (int rank = 0; rank < EDGE_SIZE; rank++) {
    for (int file = 0; file < EDGE_SIZE; file++) {
      Square s = rank * EDGE_SIZE + file;
      printf("%u ", BitBoardGetBit(b, s));
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
    return __builtin_popcountll(b);
}

Square BitBoardGetLSB(BitBoard b) {
    return __builtin_ctzll(b);
}

Square BitBoardPopLSB(BitBoard *b) {
  Square s = BitBoardGetLSB(*b);
  *b &= *b - 1;
  return s;
}

// Note: a8 is rank 0, a1 is rank 7
int BitBoardGetRank(Square s) {
  return s >> 0b11;
}

// Note: a8 is file 0, h8 is file 7
int BitBoardGetFile(Square s) {
  return s & 0b111;
}

int BitBoardGetDiagonal(Square s) {
  return BitBoardGetRank(s) + BitBoardGetFile(s);
}

int BitBoardGetAntiDiagonal(Square s) {
  return (EDGE_SIZE - 1) + BitBoardGetRank(s) - BitBoardGetFile(s);
}

BitBoard BitBoardShiftNortheast(BitBoard b) {
  return (b & ~EAST_EDGE) >> 7;
}

BitBoard BitBoardShiftNorthwest(BitBoard b) {
  return (b & ~WEST_EDGE) >> 9;
}

BitBoard BitBoardShiftSoutheast(BitBoard b) {
  return (b & ~EAST_EDGE) << 9;
}

BitBoard BitBoardShiftSouthwest(BitBoard b) {
  return (b & ~WEST_EDGE) << 7;
}
