#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  BitBoardPrint(CASTLING & KINGSIDE);
  return 0;
}