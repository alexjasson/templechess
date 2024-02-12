#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  BitBoardPrint(0xF0F0F0F0F0F0F0F0);
  BitBoardPrint(0x1F1F1F1F1F1F1F1F);
  BitBoardPrint(0x9100000000000091);
  return 0;
}