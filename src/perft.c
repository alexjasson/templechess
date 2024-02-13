#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHILDREN 256

void treeSearch(ChessBoard cb, LookupTable l, int depth, int currentDepth, long *nodeCount) {
    if (currentDepth == depth) return;

    ChessBoard children[MAX_CHILDREN];
    memset(children, 0, sizeof(children));

    ChessBoardAddChildren(cb, children, l);
    for (int i = 0; !ChessBoardIsEmpty(children[i]); i++) {
        long localCount = (currentDepth + 1 == depth - 1) ? 1 : 0;
        treeSearch(children[i], l, depth, currentDepth + 1, &localCount);

        if (currentDepth == 0) ChessBoardPrintMove(cb, children[i], localCount);
        *nodeCount += localCount;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <depth> <'FEN'>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int depth = atoi(argv[1]);
    if (depth < 0) {
        printf("Invalid depth!\n");
        exit(EXIT_FAILURE);
    }
    char* fen = argv[2];

    LookupTable l = LookupTableNew();
    ChessBoard cb = ChessBoardFromFEN(fen);
    long totalNodes = 0;

    treeSearch(cb, l, depth, 0, &totalNodes);

    printf("\nNodes searched: %ld\n", totalNodes);

    LookupTableFree(l);
    return 0;
}








