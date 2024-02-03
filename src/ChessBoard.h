#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#include "BitBoard.h"
#include "LookupTable.h"

#define EMPTY_SQUARE -1

typedef int8_t Piece;

typedef struct {
  // The squares of pieces and the pieces of squares
  BitBoard pieces[TYPE_SIZE][COLOR_SIZE];
  Piece squares[BOARD_SIZE];
  BitBoard occupancies[COLOR_SIZE + 1]; // White, Black, Union
  // Metadata
  Color turn;
  BitBoard castling;
  Square enPassant;
  int halfMoveClock;
  int moveNumber;

} ChessBoard;

ChessBoard ChessBoardFromFEN(char *fen);
ChessBoard *ChessBoardGetChildren(ChessBoard cb, LookupTable l);
void ChessBoardPrint(ChessBoard cb);

#endif