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
    ChessBoard cb = ChessBoardNew("4k3/8/8/8/8/8/8/4K3 w - - 0 1", 11);
    ChessBoardTreeSearch(l, cb);
    LookupTableFree(l);
}








