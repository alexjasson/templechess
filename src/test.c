#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    LookupTable l = LookupTableNew();
    // BitBoard b = BitBoardSetBit(EMPTY_BOARD, e6);
    // BitBoardPrint(b);
    // BitBoard c = LookupTableGetPieceAttacks(l, e7, Rook, Black, b);
    BitBoard c = LookupTableGetPawnMoves(l, e7, Black, EMPTY_BOARD);

    BitBoardPrint(c);
    LookupTableFree(l);
    return 0;
}