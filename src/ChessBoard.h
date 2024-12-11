#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#define MOVES_SIZE 218
#define PIECE_SIZE 12
#define EMPTY_PIECE 12
#define GET_PIECE(t, c) ((t << 1) | c)
#define GET_TYPE(p) (p >> 1)
#define GET_COLOR(p) (p & 1)

typedef uint8_t Piece; // 0 = White Pawn, 1 = Black Pawn, 2 = White Knight, 3 = Black Knight, etc.

typedef struct
{
  BitBoard pieces[PIECE_SIZE + 1]; // Including empty piece
  Piece squares[BOARD_SIZE];
  Color turn;
  int depth; // Start from desired depth and decrement until 0
  Square enPassant;
  BitBoard castling;
} ChessBoard;

typedef struct
{
  Square to;
  Square from;
  Piece moved;
} Move;

ChessBoard ChessBoardNew(char *fen, int depth); // Stack allocated
void ChessBoardPlayMove(ChessBoard *new, ChessBoard *old, Move move);
void ChessBoardPrintBoard(ChessBoard cb);
void ChessBoardPrintMove(Move m, long nodes);
BitBoard ChessBoardChecking(LookupTable l, ChessBoard *cb);
BitBoard ChessBoardPinned(LookupTable l, ChessBoard *cb);
BitBoard ChessBoardDiscovered(LookupTable l, ChessBoard *cb);
BitBoard ChessBoardAttacked(LookupTable l, ChessBoard *cb);

#endif