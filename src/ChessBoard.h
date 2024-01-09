#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#include "BitBoard.h"
#include "LookupTable.h"

typedef struct ChessBoard {
  BitBoard pieces[TYPE_SIZE][COLOR_SIZE];
  BitBoard occupancies[COLOR_SIZE + 1]; // White, Black, Union
  Color turn;
} ChessBoard;

ChessBoard ChessBoardFromFEN(char *fen);
ChessBoard *ChessBoardGetChildren(ChessBoard board, LookupTable l);
void ChessBoardPrint(ChessBoard board);

#endif