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
    ChessBoard cb = ChessBoardNew("r1b1kb1r/pppnpppp/5n2/8/1KN1q3/8/PPPPQPPP/R1B2BNR w kq - 0 1", 5);
    ChessBoardTreeSearch(l, cb);

    // BitBoard b = BitBoardSetBit(0, d4);
    // b = BitBoardSetBit(b, d7);
    // BitBoard c = LookupTableRookAttacks(l, d5, b);
    // BitBoardPrint(c);

    LookupTableFree(l);
}








