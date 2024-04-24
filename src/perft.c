#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void printBinary(unsigned int n) {
    if (n > 1) {
        printBinary(n / 2);
    }
    printf("%d", n % 2);
}

int main() {
    LookupTable l = LookupTableNew();
    ChessBoard cb = ChessBoardNew("rnb1kbnr/pppppppp/8/8/1K2q3/2N5/PPPPQPPP/R1B2BNR w kq - 0 1", 7);
    ChessBoardTreeSearch(l, cb);

    // BitBoard b = BitBoardSetBit(0, d4);
    // b = BitBoardSetBit(b, d7);
    // BitBoard c = LookupTableRookAttacks(l, d5, b);
    // BitBoardPrint(c);

    LookupTableFree(l);
}








