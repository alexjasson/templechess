#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    LookupTable l = LookupTableNew();
    // Bug with castling

    ChessBoard cb = ChessBoardFromFEN("rnbqkbnr/pppppppp/8/8/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1");
    printf("TEST BOARD:\n");
    ChessBoardPrint(cb);
    printf("CHILD BOARDS:\n");
    ChessBoard *children = ChessBoardGetChildren(cb, l);
    int i = 0;
    while (children[i].occupancies[Union] != EMPTY_BOARD) {
        ChessBoardPrint(children[i]);
        i++;
    }
    LookupTableFree(l);
    return 0;
}