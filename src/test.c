#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    LookupTable l = LookupTableNew();
    BitBoard b = BitBoardSetBit(EMPTY_BOARD, e5);
    BitBoard c = LookupTableGetPieceAttacks(l, e4, Rook, White, b);

    BitBoardPrint(c);
    LookupTableFree(l);
    return 0;
}