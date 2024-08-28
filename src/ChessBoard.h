#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#define PIECE_SIZE 12
#define EMPTY_PIECE 12
#define GET_PIECE(t, c) ((t << 1) | c)
#define GET_TYPE(p) (p >> 1)
#define GET_COLOR(p) (p & 1)

typedef uint8_t Piece; // 0 = White Pawn, 1 = Black Pawn, 2 = White Knight, 3 = Black Knight, etc.

typedef struct {
  BitBoard pieces[PIECE_SIZE + 1]; // Including empty piece
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
void ChessBoardPlayMove(ChessBoard *new, ChessBoard *old, Move move);
void ChessBoardPrint(ChessBoard cb);

#endif