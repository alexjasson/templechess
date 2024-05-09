#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"

#define EMPTY_PIECE 0
#define GET_PIECE(t, c) (((t << 1) | c) + 1)
#define GET_TYPE(p) ((p - 1) >> 1)
#define GET_COLOR(p) ((p - 1) & 1)

#define OUR(t) (cb->pieces[GET_PIECE(t, cb->turn)]) // Bitboard representing our pieces of type t
#define THEIR(t) (cb->pieces[GET_PIECE(t, !cb->turn)]) // Bitboard representing their pieces of type t
#define ALL (~cb->pieces[EMPTY_PIECE]) // Bitboard of all the pieces

#define BOTHSIDE_CASTLING 0b10001001
#define KINGSIDE_CASTLING 0b00001001
#define QUEENSIDE_CASTLING 0b10001000

// Given a square, returns a bitboard representing the rank of that square
#define GET_RANK(s) (SOUTH_EDGE >> (EDGE_SIZE * (EDGE_SIZE - BitBoardGetRank(s) - 1)))
#define PAWN_ATTACKS(b, c) ((c == White) ? BitBoardShiftNW(b) | BitBoardShiftNE(b) : BitBoardShiftSW(b) | BitBoardShiftSE(b))
#define PAWN_ATTACKS_LEFT(b, c) ((c == White) ? BitBoardShiftNW(b) : BitBoardShiftSE(b))
#define PAWN_ATTACKS_RIGHT(b, c) ((c == White) ? BitBoardShiftNE(b) : BitBoardShiftSW(b))
#define SINGLE_PUSH(b, c) ((c == White) ? BitBoardShiftN(b) : BitBoardShiftS(b))
#define DOUBLE_PUSH(b, c) ((c == White) ? BitBoardShiftN(BitBoardShiftN(b)) : BitBoardShiftS(BitBoardShiftS(b)))
#define ENPASSANT(b, c) (BitBoardShiftE(b) | BitBoardShiftW(b))

#define ENPASSANT_RANK(c) (BitBoard)SOUTH_EDGE >> (EDGE_SIZE * ((c * 3) + 2)) // 6th rank for black, 3rd for white
#define PROMOTING_RANK(c) (BitBoard)NORTH_EDGE << (EDGE_SIZE * ((c * 5) + 1)) // 2nd rank for black, 7th for white

typedef struct {
  BitBoard to;
  BitBoard from;
  Piece promoted; // Used in case of promotion, otherwise empty piece
} Branch;
typedef long (*TraverseFn)(LookupTable, ChessBoard *, Branch);

typedef struct {
  Square to;
  Square from;
  Piece promoted; // Used in case of promotion, otherwise empty piece
} Move;

typedef struct {
  Piece captured;
  Piece moved;
  BitBoard enPassant;
  BitBoard castling;
} UndoData;

static Color getColorFromASCII(char asciiColor);
static Piece getPieceFromASCII(char asciiPiece);
static char getASCIIFromPiece(Piece p);

static UndoData move(ChessBoard *cb, Move m);
static void undoMove(ChessBoard *cb, Move m, UndoData u);
static BitBoard getAttackedSquares(LookupTable l, ChessBoard *cb, BitBoard them);
static BitBoard getCheckingPieces(LookupTable l, ChessBoard *cb, BitBoard them, BitBoard *pinned);
static long treeSearch(LookupTable l, ChessBoard *cb, TraverseFn traverseFn);

static void printMove(Move m, long nodes);
static long traverseMoves(LookupTable l, ChessBoard *cb, Branch br);
static long printMoves(LookupTable l, ChessBoard *cb, Branch br);
static long countMoves(LookupTable l, ChessBoard *cb, Branch br);

inline static long promotionBranches(LookupTable l, ChessBoard *cb, TraverseFn traverseFn, BitBoard *data);
inline static long pieceBranches(LookupTable l, ChessBoard *cb, TraverseFn TraverseFn, BitBoard *data);
inline static long pawnBranches(LookupTable l, ChessBoard *cb, TraverseFn TraverseFn, BitBoard *data);

static Piece addPiece(ChessBoard *cb, Square s, Piece replacemen);

// Assumes FEN and depth is valid
ChessBoard ChessBoardNew(char *fen, int depth) {
  ChessBoard cb;
  memset(&cb, 0, sizeof(ChessBoard));

  // Parse pieces and squares
  for (Square s = a8; s <= h1 && *fen; fen++) {
    if (*fen == '/') continue;

    if (*fen >= '1' && *fen <= '8') {
      for (int numSquares = *fen - '0'; numSquares > 0; numSquares--) {
        cb.pieces[EMPTY_PIECE] |= BitBoardSetBit(EMPTY_BOARD, s);
        cb.squares[s] = EMPTY_PIECE;
        s++;
      }
    } else {
      Piece p = getPieceFromASCII(*fen);
      cb.pieces[p] |= BitBoardSetBit(EMPTY_BOARD, s);
      cb.squares[s] = p;
      s++;
    }
  }
  fen++;

  // Parse turn and depth
  cb.turn = getColorFromASCII(*fen);
  cb.depth = depth;
  fen += 2;

  // Parse castling
  if (*fen == '-') {
    fen++;
  } else {
    while (*fen != ' ') {
      int rank = (*fen == 'K' || *fen == 'Q') ? 7 : 0;
      BitRank flag = (*fen == 'K' || *fen == 'k') ? KINGSIDE_CASTLING : QUEENSIDE_CASTLING;
      cb.castling |= BitBoardSetBitRank(cb.castling, rank, flag);
      fen++;
    }
    fen++;
  }
  fen++;

  // Parse en passant
  if (*fen != '-') {
    int file = *fen - 'a';
    fen++;
    int rank = EDGE_SIZE - (*fen - '0');
    // Shift to 4th rank for white, 5th rank for black
    cb.enPassant = SINGLE_PUSH(BitBoardSetBit(EMPTY_BOARD, rank * EDGE_SIZE + file), !cb.turn);
  }

  return cb;
}

void ChessBoardPrint(ChessBoard cb) {
  printf("----------------------------------------------------\n");
  for (int rank = 0; rank < EDGE_SIZE; rank++) {
    for (int file = 0; file < EDGE_SIZE; file++) {
      Square s = rank * EDGE_SIZE + file;
      Piece p = cb.squares[s];
      printf("%c ", getASCIIFromPiece(p));
    }
    printf("%d\n", EDGE_SIZE - rank);
  }
  printf("a b c d e f g h\n\n");
  printf("Color: %d\n", cb.turn);
  printf("Depth: %d\n", cb.depth);
  Square enPassant = BitBoardGetLSB(cb.enPassant);
  printf("En Passant Square: %c%d\n", 'a' + (enPassant % EDGE_SIZE), EDGE_SIZE - (enPassant / EDGE_SIZE));
  printf("----------------------------------------------------\n");
}


// Assumes that asciiPiece is valid
static Piece getPieceFromASCII(char asciiPiece) {
  if (asciiPiece == '-') return EMPTY_PIECE;
  char *pieces = "PKNBRQ";
  Type t = (Type)(strchr(pieces, toupper(asciiPiece)) - pieces);
  Color c = isupper(asciiPiece) ? White : Black;
  return GET_PIECE(t, c);
}

// Assumes that piece is valid
static char getASCIIFromPiece(Piece p) {
  if (p == EMPTY_PIECE) return '-';
  char *pieces = "PKNBRQ";
  char asciiPiece = pieces[GET_TYPE(p)];
  return GET_COLOR(p) == Black ? tolower(asciiPiece) : asciiPiece;
}

static Color getColorFromASCII(char asciiColor) {
  return (asciiColor == 'w') ? White : Black;
}

// Data: {pinned, them, intersection}
inline static long promotionBranches(LookupTable l, ChessBoard *cb, TraverseFn traverseFn, BitBoard *data) {
  long nodes = 0;
  BitBoard b1, b2;
  Branch br;
  Square s;
  b1 = OUR(Pawn) & PROMOTING_RANK(cb->turn) & ~data[0];
  while (b1) {
    s = BitBoardPopLSB(&b1);
    b2 = BitBoardSetBit(EMPTY_BOARD, s);
    br.to = SINGLE_PUSH(b2, cb->turn) & ~ALL;
    br.to |= PAWN_ATTACKS(b2, cb->turn) & data[1];
    br.to &= data[2];
    br.from = b2;
    for (Type t = Knight; t <= Queen; t++) {
      br.promoted = GET_PIECE(t, cb->turn);
      nodes += traverseFn(l, cb, br);
    }
  }

  return nodes;
}

// Data: {pinned, us, intersection}
inline static long pieceBranches(LookupTable l, ChessBoard *cb, TraverseFn TraverseFn, BitBoard *data) {
  long nodes = 0;
  BitBoard b;
  Branch br;
  Square s;
  b = ~data[0] & data[1] & ~(OUR(Pawn) | OUR(King));
  while (b) {
    s = BitBoardPopLSB(&b);
    br.promoted = EMPTY_PIECE;
    br.to = LookupTableAttacks(l, s, GET_TYPE(cb->squares[s]), ALL) & data[2];
    br.from = BitBoardSetBit(EMPTY_BOARD, s);
    nodes += TraverseFn(l, cb, br);
  }

  return nodes;
}

// Data: {pinned, them, intersection}
inline static long pawnBranches(LookupTable l, ChessBoard *cb, TraverseFn TraverseFn, BitBoard *data) {
  long nodes = 0;
  BitBoard b1, b2;
  Branch br;

  b1 = OUR(Pawn) & ~(data[0] | PROMOTING_RANK(cb->turn));
  // Attacks
  br.promoted = EMPTY_PIECE;
  br.to = PAWN_ATTACKS_LEFT(b1, cb->turn) & data[1] & data[2];
  br.from = PAWN_ATTACKS_LEFT(br.to, !cb->turn);
  nodes += TraverseFn(l, cb, br);
  br.to = PAWN_ATTACKS_RIGHT(b1, cb->turn) & data[1] & data[2];
  br.from = PAWN_ATTACKS_RIGHT(br.to, !cb->turn);
  nodes += TraverseFn(l, cb, br);

  // Pushes
  b2 = SINGLE_PUSH(b1, cb->turn) & ~ALL;
  br.to = b2 & data[2];
  br.from = SINGLE_PUSH(br.to, !cb->turn);
  nodes += TraverseFn(l, cb, br);
  br.to = SINGLE_PUSH(b2 & ENPASSANT_RANK(cb->turn), cb->turn) & ~ALL & data[2];
  br.from = DOUBLE_PUSH(br.to, !cb->turn);
  nodes += TraverseFn(l, cb, br);

  return nodes;
}

// Data: {pinned, intersection} pinned is actually an intersection
inline static long enPassantBranches(LookupTable l, ChessBoard *cb, TraverseFn traverseFn, BitBoard *data) {
  long nodes = 0;
  BitBoard b;
  Branch br;
  Square s;

  b = ENPASSANT(cb->enPassant, cb->turn) & OUR(Pawn) & ~data[0];
  while (b) {
    s = BitBoardPopLSB(&b);
    // Check that the pawn is not "pseudo-pinned"
    if (LookupTableAttacks(l, BitBoardGetLSB(OUR(King)), Rook, ALL & ~BitBoardSetBit(cb->enPassant, s)) &
        GET_RANK(BitBoardGetLSB(OUR(King))) & (THEIR(Rook) | THEIR(Queen))) continue;
    br.to = cb->enPassant & data[1];
    br.from = BitBoardSetBit(EMPTY_BOARD, s);
    nodes += traverseFn(l, cb, br);
  }

  return nodes;
}

static long treeSearch(LookupTable l, ChessBoard *cb, TraverseFn traverseFn) {
  // Base cases
  if (cb->depth == 0) return 1;
  // If we are at the node before the leaf nodes and still traversing, start counting moves
  if (cb->depth == 1 && traverseFn == traverseMoves) return treeSearch(l, cb, countMoves);
  long nodes = 0;

  // Helper BitBoards/Squares
  BitBoard b1, b2, b3;
  Square s1;
  Branch br;

  // Data needed for move generation
  Square ourKing = BitBoardGetLSB(OUR(King));
  BitBoard us, them, pinned, checking, attacked;
  int numChecking;
  us = them = pinned = checking = EMPTY_BOARD;

  for (Type t = Pawn; t <= Queen; t++) us |= OUR(t);
  them = ALL & ~us;
  attacked = getAttackedSquares(l, cb, them);
  checking = getCheckingPieces(l, cb, them, &pinned);
  numChecking = BitBoardCountBits(checking);

  // Traverse king branches
  br.to = LookupTableAttacks(l, ourKing, King, ALL) & ~us & ~attacked;
  br.from = OUR(King);
  br.promoted = EMPTY_PIECE;
  nodes += traverseFn(l, cb, br);

  if (numChecking > 1) return nodes; // Double check, only king can move

  if (numChecking > 0) { // Single check

    b3 = checking | LookupTableGetSquaresBetween(l, BitBoardGetLSB(checking), ourKing); // Use b3 as checkmask

    // Traverse non pinned piece branches
    nodes += pieceBranches(l, cb, traverseFn, (BitBoard[]){pinned, us, b3});
    // Traverse non pinned, non promoting pawn branches
    nodes += pawnBranches(l, cb, traverseFn, (BitBoard[]){pinned, them, b3});
    // Traverse non pinned, promoting pawn branches
    nodes += promotionBranches(l, cb, traverseFn, (BitBoard[]){pinned, them, b3});
    // Traverse enpassant branches
    nodes += enPassantBranches(l, cb, traverseFn, (BitBoard[]){EMPTY_BOARD, ~EMPTY_BOARD});

    return nodes;
  }

  // No check

  // Traverse non pinned piece branches
  nodes += pieceBranches(l, cb, traverseFn, (BitBoard[]){pinned, us, ~us});
  // Traverse non pinned, non promoting pawn branches
  nodes += pawnBranches(l, cb, traverseFn, (BitBoard[]){pinned, them, ~EMPTY_BOARD});
  // Traverse non pinned, promoting pawn branches
  nodes += promotionBranches(l, cb, traverseFn, (BitBoard[]){pinned, them, ~EMPTY_BOARD});
  // Traverse pinned Enpassant branches
  nodes += enPassantBranches(l, cb, traverseFn, (BitBoard[]){pinned, ~EMPTY_BOARD});

  // Traverse pinned piece branches
  b1 = (OUR(Bishop) | OUR(Rook)| OUR(Queen)) & pinned;
  while (b1) {
    s1 = BitBoardPopLSB(&b1);
    // Remove moves that are not on the pin line
    br.to = LookupTableAttacks(l, s1, GET_TYPE(cb->squares[s1]), ALL)
          & LookupTableGetLineOfSight(l, ourKing, s1);
    br.from = BitBoardSetBit(EMPTY_BOARD, s1);
    nodes += traverseFn(l, cb, br);
  }

  // Traverse pinned, non promoting pawn branches - one to many mapping
  b1 = OUR(Pawn) & ~(~pinned | PROMOTING_RANK(cb->turn));
  while (b1) {
    s1 = BitBoardPopLSB(&b1);
    b2 = BitBoardSetBit(EMPTY_BOARD, s1);
    b3 = SINGLE_PUSH(b2, cb->turn) & ~ALL;
    br.to = b3;
    br.to |= SINGLE_PUSH(b3 & ENPASSANT_RANK(cb->turn), cb->turn) & ~ALL;
    br.to |= PAWN_ATTACKS(b2, cb->turn) & them;
    b3 = LookupTableGetLineOfSight(l, ourKing, s1); // The pin mask
    br.to &= b3;
    br.to |= ENPASSANT(b2, cb->turn) & cb->enPassant & SINGLE_PUSH(b3, !cb->turn);
    br.from = b2;
    nodes += traverseFn(l, cb, br);
  }

  // TODO: Add castling here

  // Traverse pinned, promoting pawn branches - one to many mapping
  b1 = OUR(Pawn) & PROMOTING_RANK(cb->turn) & pinned;
  while (b1) {
    s1 = BitBoardPopLSB(&b1);
    b2 = BitBoardSetBit(EMPTY_BOARD, s1);
    br.to = PAWN_ATTACKS(b2, cb->turn) & them;
    br.to &= LookupTableGetLineOfSight(l, ourKing, s1);
    br.from = b2;
    for (Type t = Knight; t <= Queen; t++) {
      br.promoted = GET_PIECE(t, cb->turn);
      nodes += traverseFn(l, cb, br);
    }
  }

  return nodes;
}

long traverseMoves(LookupTable l, ChessBoard *cb, Branch br) {
  Move m;
  m.from = BitBoardGetLSB(br.from);
  m.promoted = br.promoted;
  UndoData u;
  long nodes = 0;
  int oneToOne = BitBoardCountBits(br.from) - 1; // If non zero, then one-to-one mapping

  while (br.to) {
    m.to = BitBoardPopLSB(&br.to);
    if (oneToOne) m.from = BitBoardPopLSB(&br.from);
    u = move(cb, m);
    nodes += treeSearch(l, cb, traverseMoves); // Continue traversing
    undoMove(cb, m, u);
  }

  return nodes;
}

long printMoves(LookupTable l, ChessBoard *cb, Branch br) {
  Move m;
  m.from = BitBoardGetLSB(br.from);
  m.promoted = br.promoted;
  UndoData u;
  long nodes = 0, subTree;
  int oneToOne = BitBoardCountBits(br.from) - 1; // If non zero, then one-to-one mapping

  while (br.to) {
    m.to = BitBoardPopLSB(&br.to);
    if (oneToOne) m.from = BitBoardPopLSB(&br.from);
    u = move(cb, m);
    subTree = treeSearch(l, cb, traverseMoves); // Continue traversing
    nodes += subTree;
    printMove(m, subTree);
    undoMove(cb, m, u);
  }

  return nodes;
}

long countMoves(LookupTable l, ChessBoard *cb, Branch br) {
  (void) l;
  (void) cb;
  return BitBoardCountBits(br.to);
}

void ChessBoardTreeSearch(LookupTable l, ChessBoard cb) {
  long nodes = treeSearch(l, &cb, printMoves);
  printf("\nNodes: %ld\n", nodes);
}

inline static UndoData move(ChessBoard *cb, Move m) {
  int offset = m.from - m.to;

  UndoData u;
  u.moved = cb->squares[m.from];
  u.enPassant = cb->enPassant;
  u.captured = addPiece(cb, m.to, u.moved);
  addPiece(cb, m.from, EMPTY_PIECE);
  cb->enPassant = EMPTY_BOARD;

  if (GET_TYPE(u.moved) == Pawn) {
    if ((offset == 16) || (offset == -16)) {
      cb->enPassant = BitBoardSetBit(EMPTY_BOARD, m.to);
    } else if ((offset == 1) || (offset == -1)) {
      addPiece(cb, m.to + ((cb->turn) ? EDGE_SIZE : -EDGE_SIZE), u.moved);
      addPiece(cb, m.to, EMPTY_PIECE);
    } else if (m.promoted != EMPTY_PIECE) {
      addPiece(cb, m.to, m.promoted);
    }
  }

  cb->turn = !cb->turn;
  cb->depth--;

  return u;
}

inline static void undoMove(ChessBoard *cb, Move m, UndoData u) {
  int offset = m.from - m.to;

  cb->enPassant = u.enPassant;
  addPiece(cb, m.from, u.moved);
  addPiece(cb, m.to, u.captured);

  if (GET_TYPE(u.moved) == Pawn) {
    if ((offset == 1) || (offset == -1)) {
      addPiece(cb, m.to + ((cb->turn) ? -EDGE_SIZE : EDGE_SIZE), EMPTY_PIECE);
    }
  }

  cb->turn = !cb->turn;
  cb->depth++;
}

// Adds a piece to a chessboard
static Piece addPiece(ChessBoard *cb, Square s, Piece replacement) {
  BitBoard b = BitBoardSetBit(EMPTY_BOARD, s);
  Piece captured = cb->squares[s];
  cb->squares[s] = replacement;
  cb->pieces[replacement] |= b;
  cb->pieces[captured] &= ~b;

  return captured;
}

void printMove(Move m, long nodes) {
  printf("%c%d%c%d: %ld\n", 'a' + (m.from % EDGE_SIZE), EDGE_SIZE - (m.from / EDGE_SIZE), 'a' + (m.to % EDGE_SIZE), EDGE_SIZE - (m.to / EDGE_SIZE), nodes);
}

// Return the checking pieces and simultaneously update the pinned pieces bitboard
BitBoard getCheckingPieces(LookupTable l, ChessBoard *cb, BitBoard them, BitBoard *pinned) {
  Square ourKing = BitBoardGetLSB(OUR(King));
  BitBoard checking = (PAWN_ATTACKS(OUR(King), cb->turn) & THEIR(Pawn)) |
                      (LookupTableAttacks(l, ourKing, Knight, EMPTY_BOARD) & THEIR(Knight));
  BitBoard candidates = (LookupTableAttacks(l, ourKing, Bishop, them) & (THEIR(Bishop) | THEIR(Queen))) |
                        (LookupTableAttacks(l, ourKing, Rook, them) & (THEIR(Rook) | THEIR(Queen)));

  BitBoard b;
  Square s;
  while (candidates) {
    s = BitBoardPopLSB(&candidates);
    b = LookupTableGetSquaresBetween(l, ourKing, s) & ALL & ~them;
    if (b == EMPTY_BOARD) checking |= BitBoardSetBit(EMPTY_BOARD, s);
    else if ((b & (b - 1)) == EMPTY_BOARD) *pinned |= b;
  }

  return checking;
}

// Return the squares attacked by the enemy
static BitBoard getAttackedSquares(LookupTable l, ChessBoard *cb, BitBoard them) {
  BitBoard attacked, b;
  BitBoard occupancies = ALL & ~OUR(King);

  attacked = PAWN_ATTACKS(THEIR(Pawn), !cb->turn);
  b = them & ~THEIR(Pawn);
  while (b) attacked |= LookupTableAttacks(l, BitBoardPopLSB(&b), GET_TYPE(cb->squares[BitBoardGetLSB(b)]), occupancies);

  return attacked;
}