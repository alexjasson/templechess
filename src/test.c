#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    LookupTable l = LookupTableNew();
    BitBoard b = BitBoardSetBit(EMPTY_BOARD, e3);
    // b = BitBoardSetBit(b, f2);
    BitBoardPrint(b);
    BitBoard c = LookupTableGetMoves(l, f7, Pawn, Black, b);

    BitBoardPrint(c);
    LookupTableFree(l);
    return 0;
}