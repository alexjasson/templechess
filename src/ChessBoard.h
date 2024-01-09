#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#include "BitBoard.h"
#include "LookupTable.h"

typedef struct ChessBoard {
  BitBoard pieces[TYPE_SIZE][COLOR_SIZE];
  BitBoard occupancies[COLOR_SIZE + 1]; // White, Black, Union
  Color turn;

  BitBoard pieceAttacks[TYPE_SIZE][COLOR_SIZE]; // Type might not be necessary
} ChessBoard;

ChessBoard ChessBoardFromFEN(char *fen, LookupTable l);
ChessBoard *ChessBoardGetChildren(ChessBoard board, LookupTable l);
void ChessBoardPrint(ChessBoard board);

#endif