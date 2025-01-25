#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#define MOVES_SIZE 218
#define PIECE_SIZE 12
#define EMPTY_PIECE 12
#define GET_PIECE(t, c) ((t << 1) | c) // Get the piece given a type and color
#define GET_TYPE(p) (p >> 1)           // Get the type of a piece
#define GET_COLOR(p) (p & 1)           // Get the color of a piece

typedef uint8_t Piece; // 0 = White Pawn, 1 = Black Pawn, 2 = White Knight, 3 = Black Knight, etc.

/*
 * Representation of a chess board. Note that castling rights are representated as a set of
 * squares where if the original square of a king and the original square of a rook is present,
 * we have castling rights for this color/side.
 */
typedef struct
{
  BitBoard pieces[PIECE_SIZE + 1]; // A set of squares for each piece, including empty pieces
  Piece squares[BOARD_SIZE];       // A piece for each square, including empty pieces
  Color turn;
  Square enPassant;
  BitBoard castling;
  int depth; // Start from desired depth and decrement until 0
} ChessBoard;

/*
 * Representation of a move on a chess board
 */
typedef struct
{
  Square to;
  Square from;
  Piece moved;
} Move;

/*
 * Creates a new chess board with the given FEN string and depth
 */
ChessBoard ChessBoardNew(char *fen, int depth); // Stack allocated

/*
 * Given an old board and a new board, copy the old board and play the move on the new board
 */
ChessBoard ChessBoardPlayMove(ChessBoard *old, Move move);

/*
 * Prints a chess board to stdout
 */
void ChessBoardPrintBoard(ChessBoard cb);

/*
 * Prints a move to stdout
 */
void ChessBoardPrintMove(Move m, long nodes);

/*
 * Given a chess board, returns a set of squares representing their pieces that are giving check
 */
BitBoard ChessBoardChecking(LookupTable l, ChessBoard *cb);

/*
 * Given a chess board, returns a set of squares representing our pieces that are pinned
 */
BitBoard ChessBoardPinned(LookupTable l, ChessBoard *cb);

/*
 * Given a chess board, returns a set of squares representing the squares that are attacked by their pieces
 */
BitBoard ChessBoardAttacked(LookupTable l, ChessBoard *cb);

#endif