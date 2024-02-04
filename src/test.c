#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    LookupTable l = LookupTableNew();

    ChessBoard cb = ChessBoardFromFEN("1K3r2/3QP3/1N6/8/2R5/4k3/8/8 w - - 0 1");
    ChessBoardPrint(cb);

    ChessBoard *children = ChessBoardGetChildren(cb, l);
    int i = 0;
    while (children[i].occupancies[Union] != EMPTY_BOARD) {
        ChessBoardPrint(children[i]);
        i++;
    }
    LookupTableFree(l);
    return 0;
}