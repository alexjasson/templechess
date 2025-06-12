#ifndef CHESSBOARD_H
#define CHESSBOARD_H

/*
 * Representation of a chess board. Note that castling rights are representated as a set of
 * squares where if the original square of a king and the original square of a rook is present,
 * we have castling rights for this color/side.
 */
typedef struct
{
  BitBoard types[TYPE_SIZE];       // A set of squares for each piece type (Pawn, King, Knight, Bishop, Rook, Queen)
  BitBoard colors[COLOR_SIZE];     // A set of squares for each color (White, Black)
  Type squares[BOARD_SIZE];        // A piece type for each square, Empty if no piece
  Color turn;
  Square enPassant;
  BitBoard castling;
} ChessBoard;

/*
 * Representation of a piece on a chess board
 */
typedef struct
{
  Type type;
  Square square;
} Piece;

/*
 * Representation of a move on a chess board
 */
typedef struct
{
  Piece from;        // piece and origin square before the move
  Piece to;          // piece and destination square after the move (includes promotion)
  Piece captured;    // captured piece and its square (Empty type if no capture)
  Square enPassant;  // en passant square before the move
  BitBoard castling; // castling rights before the move
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
 * Play a move on the given board in-place, recording undo info in Move
 */
void ChessBoardPlayMove(ChessBoard *cb, Move m);

/*
 * Undo a move previously played with ChessBoardPlayMove
 */
void ChessBoardUndoMove(ChessBoard *cb, Move m);

/*
 * Prints a chess board to stdout
 */
void ChessBoardPrintBoard(ChessBoard cb);

/*
 * Prints a move to stdout
 */
void ChessBoardPrintMove(Move m);

/*
 * Given a chess board, return a new chess board where the turn is passed to the other color
 */
ChessBoard ChessBoardFlip(ChessBoard *cb);

// Accessor functions for ChessBoard properties
static inline int ChessBoardKingSide(ChessBoard *cb)  { return !(~cb->castling & (KINGSIDE_CASTLING & ((cb->turn == White) ? SOUTH_EDGE : NORTH_EDGE))); }
static inline int ChessBoardQueenSide(ChessBoard *cb) { return !(~cb->castling & (QUEENSIDE_CASTLING & ((cb->turn == White) ? SOUTH_EDGE : NORTH_EDGE)));}
static inline Color ChessBoardColor(ChessBoard *cb)            { return cb->turn; }
static inline Square ChessBoardEnPassant(ChessBoard *cb)       { return cb->enPassant; }
static inline BitBoard ChessBoardCastling(ChessBoard *cb)      { return cb->castling; }
static inline Type ChessBoardSquare(ChessBoard *cb, Square s)  { return cb->squares[s]; }
static inline BitBoard ChessBoardOur(ChessBoard *cb, Type t)   { return cb->types[t] & cb->colors[cb->turn]; }
static inline BitBoard ChessBoardTheir(ChessBoard *cb, Type t) { return cb->types[t] & cb->colors[!cb->turn]; }
static inline BitBoard ChessBoardAll(ChessBoard *cb)           { return cb->colors[White] | cb->colors[Black]; }
static inline BitBoard ChessBoardUs(ChessBoard *cb)            { return cb->colors[cb->turn]; }
static inline BitBoard ChessBoardThem(ChessBoard *cb)          { return cb->colors[!cb->turn]; }

#endif