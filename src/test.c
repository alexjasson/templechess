#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    LookupTable l = LookupTableNew();
    BitBoard c = LookupTableGetPieceAttacks(l, e4, Knight, White, EMPTY_BOARD);

    BitBoardPrint(c);
    LookupTableFree(l);
    return 0;
}