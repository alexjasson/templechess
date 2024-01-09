#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include <stdio.h>

int main() {
    LookupTable l = LookupTableNew();
    BitBoard c = BitBoardSetBit(0, d5);
    BitBoard b = LookupTableGetPieceAttacks(l, e4, Queen, White, c);
    BitBoardPrint(b);
    ChessBoard cb = ChessBoardFromFEN("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR w", l);
    ChessBoardPrint(cb);
    // ChessBoard *children = ChessBoardGetChildren(cb, l);
    BitBoardPrint(cb.pieceAttacks[Queen][White]);
    if (cb.turn) printf("YEET");

    LookupTableFree(l);

    return 0;
}
