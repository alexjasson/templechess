#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#define PIECE_SIZE 12

typedef uint8_t Piece;

typedef struct {
  BitBoard pieces[PIECE_SIZE + 1];
  Piece squares[BOARD_SIZE];

  Color turn;
  int depth; // Start from desired depth and decrement until 0
  BitBoard enPassant;
  BitBoard castling;
} ChessBoard;

ChessBoard ChessBoardNew(char *fen, int depth); // ChessBoard is stack allocated
void ChessBoardPrint(ChessBoard cb);
void ChessBoardTreeSearch(LookupTable l, ChessBoard cb);

#endif