#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#define PIECE_SIZE 12

typedef uint8_t Piece;

typedef struct {
  BitBoard pieces[PIECE_SIZE + 1];
  Piece squares[BOARD_SIZE];

  Color turn;
  int depth; // Start from desired depth and decrement until 0
  Square enPassant;
  BitBoard castling;
} ChessBoard;

typedef struct {
  Square to;
  Square from;
  Type moved;
} Move;

ChessBoard ChessBoardNew(char *fen, int depth); // ChessBoard is stack allocated
void ChessBoardPrint(ChessBoard cb);
long ChessBoardTreeSearch(ChessBoard cb);
void ChessBoardPlayMove(ChessBoard *new, ChessBoard *old, Move move);

#endif