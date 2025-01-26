#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#define PIECE_SIZE 12
#define EMPTY_PIECE 12
#define GET_PIECE(t, c) ((t << 1) | c) // Get the piece given a type and color
#define GET_TYPE(p) (p >> 1)           // Get the type of a piece
#define GET_COLOR(p) (p & 1)           // Get the color of a piece

#define OUR(t) (cb->pieces[GET_PIECE(t, cb->turn)])                                     // Bitboard representing our pieces of type t
#define THEIR(t) (cb->pieces[GET_PIECE(t, !cb->turn)])                                  // Bitboard representing their pieces of type t
#define ALL (~cb->pieces[EMPTY_PIECE])                                                  // Bitboard of all the pieces
#define US (OUR(Pawn) | OUR(Knight) | OUR(Bishop) | OUR(Rook) | OUR(Queen) | OUR(King)) // Bitboard of all our pieces
#define THEM (ALL & ~US)                                                                // Bitboard of all their pieces

/*
 * A piece is a number from 0 to 11 representing a piece on a chess board.
 * 0 = White Pawn, 1 = Black Pawn, 2 = White Knight, 3 = Black Knight, etc.
 */
typedef uint8_t Piece;

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
 * Creates a new chess board with the given FEN string
 */
ChessBoard ChessBoardNew(char *fen); // Stack allocated

/*
 * Given an old board, play the move on the old board and return the new board
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