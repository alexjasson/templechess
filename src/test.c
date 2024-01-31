#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    LookupTable l = LookupTableNew();
    BitBoard b = BitBoardSetBit(EMPTY_BOARD, e6);
    BitBoardPrint(b);
    BitBoard c = LookupTableGetPieceAttacks(l, f7, Bishop, Black, b);
    // BitBoard c = LookupTableGetPawnMoves(l, e7, Black, b);

    BitBoardPrint(c);
    LookupTableFree(l);
    return 0;
}