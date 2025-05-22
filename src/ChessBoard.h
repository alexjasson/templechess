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
 * Representation of a move on a chess board
 */
typedef struct
{
  Square to;
  Square from;
  Type type;
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

/*
 * Returns 1 if the side to move has king-side castling rights, 0 otherwise
 */
int ChessBoardKingSide(ChessBoard *cb);

/*
 * Returns 1 if the side to move has queen-side castling rights, 0 otherwise
 */
int ChessBoardQueenSide(ChessBoard *cb);

static inline Color ChessBoardColor(ChessBoard *cb)            { return cb->turn; }
static inline Square ChessBoardEnPassant(ChessBoard *cb)       { return cb->enPassant; }
static inline Type ChessBoardSquare(ChessBoard *cb, Square s)  { return cb->squares[s]; }
static inline BitBoard ChessBoardOur(ChessBoard *cb, Type t)   { return cb->types[t] & cb->colors[cb->turn]; }
static inline BitBoard ChessBoardTheir(ChessBoard *cb, Type t) { return cb->types[t] & cb->colors[!cb->turn]; }
static inline BitBoard ChessBoardAll(ChessBoard *cb)           { return cb->colors[White] | cb->colors[Black]; }
static inline BitBoard ChessBoardUs(ChessBoard *cb)            { return cb->colors[cb->turn]; }
static inline BitBoard ChessBoardThem(ChessBoard *cb)          { return cb->colors[!cb->turn]; }

#endif