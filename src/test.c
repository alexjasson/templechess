#include "BitBoard.h"
#include "AttackTable.h"

int main() {
    BitBoard b = 0, c = 0, d = 0;
    AttackTable a = AttackTableNew();
    b = BitBoardSetBit(b, a1);
    b = BitBoardSetBit(b, h8);
    // c = AttackTableGetPieceAttacks(a, Knight, White, e4);
    d = AttackTableGetPieceAttacks(a, Rook, White, a1);
    BitBoardPrint(b);
    BitBoardPrint(c);
    BitBoardPrint(d);

    AttackTableFree(a);

    return 0;
}
