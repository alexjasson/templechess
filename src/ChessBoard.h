#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#define OUR(t) (cb->pieces[GET_PIECE(t, cb->turn)])                                     // Bitboard representing our pieces of type t
#define THEIR(t) (cb->pieces[GET_PIECE(t, !cb->turn)])                                  // Bitboard representing their pieces of type t
#define ALL (~cb->pieces[EMPTY_PIECE])                                                  // Bitboard of all the pieces
#define US (OUR(Pawn) | OUR(Knight) | OUR(Bishop) | OUR(Rook) | OUR(Queen) | OUR(King)) // Bitboard of all our pieces
#define THEM (ALL & ~US)                                                                // Bitboard of all their pieces

#define GET_RANK(s) (SOUTH_EDGE >> (EDGE_SIZE * (EDGE_SIZE - BitBoardGetRank(s) - 1))) // BitBoard representing the rank of a specific square
#define ENPASSANT_RANK(c) (BitBoard) SOUTH_EDGE >> (EDGE_SIZE * ((c * 3) + 2))         // BitBoard representing the enpassant rank given a color
#define PROMOTING_RANK(c) (BitBoard)((c == White) ? NORTH_EDGE : SOUTH_EDGE)           // BitBoard representing the promotion rank given a color
#define BACK_RANK(c) (BitBoard)((c == White) ? SOUTH_EDGE : NORTH_EDGE)                // BitBoard representing the back rank given a color

// Masks used for castling
#define KINGSIDE_CASTLING 0x9000000000000090
#define QUEENSIDE_CASTLING 0x1100000000000011
#define QUEENSIDE 0x1F1F1F1F1F1F1F1F
#define KINGSIDE 0xF0F0F0F0F0F0F0F0
#define ATTACK_MASK 0x6c0000000000006c
#define OCCUPANCY_MASK 0x6e0000000000006e

// Returns a bitboard representing a set of moves given a set of pawns and a color
#define PAWN_ATTACKS(b, c) ((c == White) ? BitBoardShiftNW(b) | BitBoardShiftNE(b) : BitBoardShiftSW(b) | BitBoardShiftSE(b))
#define PAWN_ATTACKS_LEFT(b, c) ((c == White) ? BitBoardShiftNW(b) : BitBoardShiftSE(b))
#define PAWN_ATTACKS_RIGHT(b, c) ((c == White) ? BitBoardShiftNE(b) : BitBoardShiftSW(b))
#define SINGLE_PUSH(b, c) ((c == White) ? BitBoardShiftN(b) : BitBoardShiftS(b))
#define DOUBLE_PUSH(b, c) ((c == White) ? BitBoardShiftN(BitBoardShiftN(b)) : BitBoardShiftS(BitBoardShiftS(b)))

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
void ChessBoardPlayMove(ChessBoard *new, ChessBoard *old, Move move);

/*
 * Given an old board a new board, copy the old and pass the turn to other player
 */
void ChessBoardPassMove(ChessBoard *new, ChessBoard *old);

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