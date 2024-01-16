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
  int count = 0;
  while (b) {
    count++;
    b &= b - 1;
  }
  return count;
}

int BitBoardGetRank(Square s) {
  return s >> 0b11;
}

int BitBoardGetFile(Square s) {
  return s & 0b111;
}


Square BitBoardLeastSignificantBit(BitBoard b) {
  return BitBoardCountBits((b & -b) - 1);
}

BitBoard BitBoardShiftNorth(BitBoard b, int magnitude) {
  return b >> EDGE_SIZE * magnitude;
}

BitBoard BitBoardShiftEast(BitBoard b, int magnitude) {
  return b << magnitude;
}

BitBoard BitBoardShiftSouth(BitBoard b, int magnitude) {
  return b << EDGE_SIZE * magnitude;
}

BitBoard BitBoardShiftWest(BitBoard b, int magnitude) {
  return b >> magnitude;
}