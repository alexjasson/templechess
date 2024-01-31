#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    LookupTable l = LookupTableNew();

    BitBoard c = LookupTableGetLineOfSight(l, b8, h2);
    BitBoard d = LookupTableGetSquaresBetween(l, b8, h2);

    BitBoardPrint(c);
    BitBoardPrint(d);
    LookupTableFree(l);
    return 0;
}