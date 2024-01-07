#include "BitBoard.h"
#include "LookupTable.h"
#include <stdio.h>

int main() {
    //BitBoard b = 0;
    //BitBoard c = 0;
    LookupTable l = LookupTableNew();
    BitBoard c = BitBoardSetBit(0, d4);
    //BitBoard d = 0xFFFFFFFFFFFFFFFF;
    BitBoard b = LookupTableGetPieceAttacks(l, a2, Knight, White, c);
    BitBoardPrint(b);

    // b = BitBoardSetBit(b, c6);
    // b = BitBoardSetBit(b, d5);
    // b = BitBoardSetBit(b, g2);
    // c = AttackTableGetPieceAttacks(a, Bishop, White, e4, b);
    // BitBoardPrint(c);

    LookupTableFree(l);

    return 0;
}
