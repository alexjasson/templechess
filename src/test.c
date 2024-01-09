#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"

int main() {
    LookupTable l = LookupTableNew();
    BitBoard c = BitBoardSetBit(0, d5);
    BitBoard b = LookupTableGetPieceAttacks(l, e4, Queen, White, c);
    BitBoardPrint(b);
    ChessBoard cb = ChessBoardFromFEN("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR w");
    ChessBoardPrint(cb);
    ChessBoard *children = ChessBoardGetChildren(cb, l);
    for (int i = 0; i < 15; i++) {
        ChessBoardPrint(children[i]);
    }

    LookupTableFree(l);

    return 0;
}
