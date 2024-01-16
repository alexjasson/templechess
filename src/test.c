#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include <stdio.h>

int main() {
    LookupTable l = LookupTableNew();
    BitBoard c = BitBoardSetBit(0, d5);
    BitBoard b = LookupTableGetPieceAttacks(l, e4, Queen, White, c);
    BitBoardPrint(b);
    ChessBoard cb = ChessBoardFromFEN("rnbqkbnr/pppppppp/4N3/8/8/8/PPPPPPPP/R1BQKBNR w KQkq h6", l);
    // printf("here?\n");
    ChessBoardPrint(cb);
    ChessBoard *children = ChessBoardGetChildren(cb, l);
    for (int i = 0; i < 12; i++) {
        ChessBoardPrint(children[i]);
        // BitBoardPrint(children[i].occupancies[Union]);
    }
    // printf("Actual attacks:\n");
    // BitBoardPrint(cb->attacks[White]);
    // printf("Actual occupancies:\n");
    // BitBoardPrint(cb->occupancies[Union]);
    // printf("Actual castling:\n");
    // BitBoardPrint(cb->castling[Black]);
    // printf("Actual enpassant:\n");
    // printf("%d\n", cb->enPassant);
    // // BitBoardPrint(compressEnPassant(cb.enPassant, cb.turn));
    // if (cb->turn) printf("YEET\n");

    LookupTableFree(l);
    // BitBoard g = EMPTY_BOARD;
    // BitBoardPrint(0x0000000000000011);
    // g = BitBoardSetBit(g, a1);
    // BitBoardPrint(g);
    return 0;
}
