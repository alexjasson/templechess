#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOARD "k7/8/1K6/8/2N5/5q2/8/8 b - - 0 1"

int main() {
    LookupTable l = LookupTableNew();
    ChessBoard cb = ChessBoardNew(BOARD, 3);
    ChessBoardTreeSearch(l, cb);

    LookupTableFree(l);
}








