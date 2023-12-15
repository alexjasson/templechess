#include "BitBoard.h"
#include "AttackTable.h"

int main() {
    BitBoard b = 0, c = 0, d = 0;
    AttackTable a = AttackTableNew();
    b = BitBoardSetBit(b, a1);
    b = BitBoardSetBit(b, h8);
    c = AttackTableGetKnightAttacks(a, e4);
    d = AttackTableGetWhitePawnAttacks(a, e4);
    BitBoardPrint(b);
    BitBoardPrint(c);
    BitBoardPrint(d);

    return 0;
}
