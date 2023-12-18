#include "BitBoard.h"
#include "AttackTable.h"

int main() {
    // BitBoard b = 0;
    BitBoard c = 0;
    //b = BitBoardSetBit(b, e4);
    AttackTable a = AttackTableNew();
    // b = BitBoardSetBit(b, a2);
    // b = BitBoardSetBit(b, g1);
    c = AttackTableGetPieceAttacks(a, Bishop, White, e2);
    BitBoardPrint(c);
    //b = BitBoardShiftBit(b, East, 4);
    // BitBoardPrint(b);
    // BitBoardPrint(FILE_H << 1);
    // b = BitBoardSetBit(b, h8);
    // c = AttackTableGetPieceAttacks(a, Knight, White, e4);
    // d = AttackTableGetPieceAttacks(a, Rook, White, a1);
    // BitBoardPrint(b);
    // b = BitBoardShiftBit(b, Northwest, 3);
    // BitBoardPrint(b);
    // BitBoard c = FILE_H;
    // BitBoardPrint(c);
    // BitBoardPrint(c);
    // BitBoardPrint(d);

    AttackTableFree(a);

    return 0;
}
