#include "BitBoard.h"
#include "AttackTable.h"
#include <stdio.h>

int main() {
    //BitBoard b = 0;
    //BitBoard c = 0;
    AttackTable a = AttackTableNew();
    BitBoard c = BitBoardSetBit(0, e5);
    BitBoard b = AttackTableGetPieceAttacks(a, Pawn, Black, e2, c);
    BitBoardPrint(b);

    // b = BitBoardSetBit(b, c6);
    // b = BitBoardSetBit(b, d5);
    // b = BitBoardSetBit(b, g2);
    // c = AttackTableGetPieceAttacks(a, Bishop, White, e4, b);
    // BitBoardPrint(c);

    AttackTableFree(a);

    return 0;
}
