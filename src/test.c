#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define START_POSITION "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

void traverseChildren(ChessBoard cb, LookupTable l, int depth, int currentDepth, int *nodeCount, int isFirstCall) {
    ChessBoard *children = ChessBoardGetChildren(cb, l);
    if (!children) return; // Guard against no children

    int localCount = 0; // Local count for this recursion level

    for (int i = 0; children[i].occupancies[Union] != EMPTY_BOARD; i++) {
        if (currentDepth + 1 == depth) {
            // Increment at the target depth
            localCount++;
        } else if (currentDepth + 1 < depth) {
            // Continue deeper if not yet at target depth
            traverseChildren(children[i], l, depth, currentDepth + 1, &localCount, 0);
        }

        if (currentDepth == 0 && isFirstCall) {
            // Print move and reset local count after printing for the first call (root children)
            ChessBoardPrintMove(cb, children[i]);
            printf(": %d\n", localCount);
            // ChessBoardPrint(cb);
            // ChessBoardPrint(children[i]);
            // BitBoardPrint((cb.occupancies[cb.turn] ^ children[i].occupancies[!children[i].turn]) & cb.occupancies[cb.turn]);
            // BitBoardPrint((cb.occupancies[cb.turn] ^ children[i].occupancies[!children[i].turn]) & children[i].occupancies[!children[i].turn]);
            // BitBoardPrint(children[i].occupancies[Union]);
            // printf("Turn %d\n", children[i].turn);
            *nodeCount += localCount; // Accumulate total nodes visited at target depth
            localCount = 0; // Reset local count after handling each child
        }
    }

    if (currentDepth != 0 && !isFirstCall) {
        // Update the node count for non-root levels
        *nodeCount += localCount;
    }

    free(children); // Assuming dynamically allocated children
}

int main() {
    LookupTable l = LookupTableNew();
    ChessBoard cb = ChessBoardFromFEN(START_POSITION);
    printf("TEST BOARD:\n");
    ChessBoardPrint(cb);
    // ChessBoard cb2 = ChessBoardFromFEN("rnbqkb1r/pppppp1p/6Pn/8/8/8/PPPPPPP1/RNBQKBNR b KQkq - 0 1");

    int depth = 7; // Set the desired depth here
    printf("\nTRAVERSING TO DEPTH: %d\n", depth);
    int totalNodes = 0; // Initialize total node count

    // Start traversal with isFirstCall = 1 to indicate it's the first call
    traverseChildren(cb, l, depth, 0, &totalNodes, 1);

    printf("\nTotal nodes visited at depth %d: %d\n", depth, totalNodes);
    // ChessBoardPrintMove(cb, cb2);

    LookupTableFree(l);
    return 0;
}







