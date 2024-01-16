#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#include "BitBoard.h"
#include "LookupTable.h"

typedef struct {
  // Data - Represents a position and must be given
  BitBoard pieces[TYPE_SIZE][COLOR_SIZE];
  Color turn;
  Square enPassant;
  BitBoard castling;
  // Metadata - Determined by the data
  BitBoard occupancies[COLOR_SIZE + 1]; // White, Black, Union
  BitBoard attacks[COLOR_SIZE];
} ChessBoard;

ChessBoard ChessBoardFromFEN(char *fen, LookupTable l);
ChessBoard *ChessBoardGetChildren(ChessBoard cb, LookupTable l);
void ChessBoardPrint(ChessBoard cb);

#endif