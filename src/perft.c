#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOARD "k1q5/8/1K6/8/2N5/5n2/1R6/B7 b - - 0 1"

int main(int argc, char **argv) {
    // Check arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <fen> <depth>\n", argv[0]);
        exit(1);
    }

    LookupTable l = LookupTableNew();
    ChessBoard cb = ChessBoardNew(argv[1], atoi(argv[2]));
    ChessBoardTreeSearch(l, cb);

    LookupTableFree(l);
}








