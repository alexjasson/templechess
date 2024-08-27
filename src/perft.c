#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include "Branch.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    // Check arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <fen> <depth>\n", argv[0]);
        exit(1);
    }

    ChessBoard cb = ChessBoardNew(argv[1], atoi(argv[2]));
    long nodes = BranchTreeSearch(&cb);
    printf("\nNodes searched: %ld\n", nodes);
}








