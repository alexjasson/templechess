#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#define EMPTY_PIECE 12
#define GET_PIECE(t, c) ((t << 1) | c) // Get the piece given a type and color
#define GET_TYPE(p) (p >> 1)           // Get the type of a piece
#define GET_COLOR(p) (p & 1)           // Get the color of a piece

#define OUR(t) ((cb)->pieceTypeBB[t] & (cb)->colorBB[ChessBoardColor(cb)])
#define THEIR(t) ((cb)->pieceTypeBB[t] & (cb)->colorBB[!ChessBoardColor(cb)])
#define ALL ((cb)->colorBB[White] | (cb)->colorBB[Black])
#define US ((cb)->colorBB[ChessBoardColor(cb)])
#define THEM ((cb)->colorBB[!ChessBoardColor(cb)])

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
  BitBoard pieceTypeBB[TYPE_SIZE]; // A set of squares for each piece type (Pawn, King, Knight, Bishop, Rook, Queen)
  BitBoard colorBB[COLOR_SIZE];            // A set of squares for each color (White, Black)
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
 * Given a chess board, return it's FEN string representation
 */
char *ChessBoardToFEN(ChessBoard *cb);

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
void ChessBoardPrintMove(Move m);

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

/*
 * Given a chess board, return a new chess board where the turn is passed to the other color
 */
ChessBoard ChessBoardFlip(ChessBoard *cb);

int ChessBoardKingSide(ChessBoard *cb);  // Returns 1 if the side to move has king-side castling rights, 0 otherwise
int ChessBoardQueenSide(ChessBoard *cb); // Returns 1 if the side to move has queen-side castling rights, 0 otherwise
static inline Color ChessBoardColor(ChessBoard *cb) { return cb->turn; } // Returns the color of the side to move
static inline Square ChessBoardEnPassant(ChessBoard *cb) { return cb->enPassant; } // Returns the square of the en passant
static inline Piece ChessBoardSquare(ChessBoard *cb, Square s) { return cb->squares[s]; } // Returns the piece on the given square
/* Returns the bitboard of the given piece (type and color combined) */
static inline BitBoard ChessBoardPieces(ChessBoard *cb, Piece p)
{
  return (cb->pieceTypeBB[GET_TYPE(p)] & cb->colorBB[GET_COLOR(p)]);
}

#endif