#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    // Check arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <fen> <depth>\n", argv[0]);
        exit(1);
    }

    LookupTable l = LookupTableNew();
    ChessBoard cb = ChessBoardNew(argv[1], atoi(argv[2]));
    long nodes = ChessBoardTreeSearch(l, cb);
    printf("\nNodes searched: %ld\n", nodes);
    LookupTableFree(l);
}








