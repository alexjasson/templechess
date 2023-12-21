#include "BitBoard.h"
#include "AttackTable.h"
#include <stdio.h>

#define FILE_A 0x0101010101010101
#define FILE_H 0x8080808080808080
#define RANK_1 0x00000000000000FF
#define RANK_8 0xFF00000000000000

int main() {
    //BitBoard b = 0;
    //BitBoard c = 0;
    AttackTable a = AttackTableNew();
    BitBoard c = BitBoardSetBit(0, e5);
    BitBoard b = AttackTableGetPieceAttacks(a, Rook, White, e4, c);
    BitBoardPrint(b);

    // b = BitBoardSetBit(b, c6);
    // b = BitBoardSetBit(b, d5);
    // b = BitBoardSetBit(b, g2);
    // c = AttackTableGetPieceAttacks(a, Bishop, White, e4, b);
    // BitBoardPrint(c);

    AttackTableFree(a);

    return 0;
}
