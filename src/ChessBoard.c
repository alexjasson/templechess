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

// Masks used for castling
#define KINGSIDE_CASTLING 0x9000000000000090
#define QUEENSIDE_CASTLING 0x1100000000000011
#define QUEENSIDE 0x1F1F1F1F1F1F1F1F
#define KINGSIDE 0xF0F0F0F0F0F0F0F0
#define ATTACK_MASK 0x6c0000000000006c
#define OCCUPANCY_MASK 0x6e0000000000006e

// Given a square, returns a bitboard representing the rank of that square
#define GET_RANK(s) (SOUTH_EDGE >> (EDGE_SIZE * (EDGE_SIZE - BitBoardGetRank(s) - 1)))
#define PAWN_ATTACKS(b, c) ((c == White) ? BitBoardShiftNW(b) | BitBoardShiftNE(b) : BitBoardShiftSW(b) | BitBoardShiftSE(b))
#define PAWN_ATTACKS_LEFT(b, c) ((c == White) ? BitBoardShiftNW(b) : BitBoardShiftSE(b))
#define PAWN_ATTACKS_RIGHT(b, c) ((c == White) ? BitBoardShiftNE(b) : BitBoardShiftSW(b))
#define SINGLE_PUSH(b, c) ((c == White) ? BitBoardShiftN(b) : BitBoardShiftS(b))
#define DOUBLE_PUSH(b, c) ((c == White) ? BitBoardShiftN(BitBoardShiftN(b)) : BitBoardShiftS(BitBoardShiftS(b)))
#define ENPASSANT(b) (BitBoardShiftE(b) | BitBoardShiftW(b))

#define ENPASSANT_RANK(c) (BitBoard)SOUTH_EDGE >> (EDGE_SIZE * ((c * 3) + 2)) // 6th rank for black, 3rd for white
#define PROMOTING_RANK(c) (BitBoard)NORTH_EDGE << (EDGE_SIZE * ((c * 5) + 1)) // 2nd rank for black, 7th for white
#define BACK_RANK(c) (BitBoard)((c == White) ? SOUTH_EDGE : NORTH_EDGE)

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

static Piece addPiece(ChessBoard *cb, Square s, Piece replacemen);
inline static UndoData move(ChessBoard *cb, Move m);
inline static void undoMove(ChessBoard *cb, Move m, UndoData u);
static BitBoard getAttackedSquares(LookupTable l, ChessBoard *cb, BitBoard them);
static BitBoard getCheckingPieces(LookupTable l, ChessBoard *cb, BitBoard them, BitBoard *pinned);
static long treeSearch(LookupTable l, ChessBoard *cb, TraverseFn traverseFn);

static void printMove(Piece moved, Move m, long nodes);
static long traverseMoves(LookupTable l, ChessBoard *cb, Branch br);
static long printMoves(LookupTable l, ChessBoard *cb, Branch br);
static long countMoves(LookupTable l, ChessBoard *cb, Branch br);

inline static long promotionBranches(LookupTable l, ChessBoard *cb, TraverseFn traverseFn, BitBoard intersection[]);
inline static long pieceBranches(LookupTable l, ChessBoard *cb, TraverseFn TraverseFn, BitBoard intersection[]);
inline static long pawnBranches(LookupTable l, ChessBoard *cb, TraverseFn TraverseFn, BitBoard intersection[]);
inline static long enPassantBranches(LookupTable l, ChessBoard *cb, TraverseFn traverseFn, BitBoard intersection[]);
inline static long pieceBranchesPinned(LookupTable l, ChessBoard *cb, TraverseFn TraverseFn, BitBoard intersection[]);
inline static long pawnBranchesPinned(LookupTable l, ChessBoard *cb, TraverseFn TraverseFn, BitBoard intersection[]);
inline static long promotingBranchesPinned(LookupTable l, ChessBoard *cb, TraverseFn TraverseFn, BitBoard intersection[]);
inline static long kingBranches(LookupTable l, ChessBoard *cb, TraverseFn traverseFn, BitBoard intersection[]);
inline static long castlingBranches(LookupTable l, ChessBoard *cb, TraverseFn traverseFn, BitBoard intersection[]);


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
    fen += 2;
  } else {
    while (*fen != ' ') {
      Color c = (*fen == 'K' || *fen == 'Q') ? 0 : 7;
      BitBoard castlingSquares = (*fen == 'K' || *fen == 'k') ? KINGSIDE_CASTLING : QUEENSIDE_CASTLING;
      cb.castling |= (castlingSquares & BACK_RANK(c));
      fen++;
    }
    fen++;
  }

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
  for (int rank = 0; rank < EDGE_SIZE; rank++) {
    for (int file = 0; file < EDGE_SIZE; file++) {
      Square s = rank * EDGE_SIZE + file;
      Piece p = cb.squares[s];
      printf("%c ", getASCIIFromPiece(p));
    }
    printf("%d\n", EDGE_SIZE - rank);
  }
  printf("a b c d e f g h\n\n");
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

inline static long promotionBranches(LookupTable l, ChessBoard *cb, TraverseFn traverseFn, BitBoard intersection[]) {
  long nodes = 0;
  BitBoard b1, b2;
  Branch br;
  Square s;
  b1 = OUR(Pawn) & PROMOTING_RANK(cb->turn) & intersection[0];
  while (b1) {
    s = BitBoardPopLSB(&b1);
    b2 = BitBoardSetBit(EMPTY_BOARD, s);
    br.to = SINGLE_PUSH(b2, cb->turn) & ~ALL;
    br.to |= PAWN_ATTACKS(b2, cb->turn) & intersection[1];
    br.to &= intersection[2];
    br.from = b2;
    for (Type t = Knight; t <= Queen; t++) {
      br.promoted = GET_PIECE(t, cb->turn);
      nodes += traverseFn(l, cb, br);
    }
  }

  return nodes;
}

inline static long pieceBranches(LookupTable l, ChessBoard *cb, TraverseFn TraverseFn, BitBoard intersection[]) {
  long nodes = 0;
  BitBoard b;
  Branch br;
  Square s;
  b = intersection[0] & intersection[1] & ~(OUR(Pawn) | OUR(King));
  while (b) {
    s = BitBoardPopLSB(&b);
    br.promoted = EMPTY_PIECE;
    br.to = LookupTableAttacks(l, s, GET_TYPE(cb->squares[s]), ALL) & intersection[2];
    br.from = BitBoardSetBit(EMPTY_BOARD, s);
    nodes += TraverseFn(l, cb, br);
  }

  return nodes;
}

inline static long pawnBranches(LookupTable l, ChessBoard *cb, TraverseFn TraverseFn, BitBoard intersection[]) {
  long nodes = 0;
  BitBoard b1, b2;
  Branch br;

  b1 = OUR(Pawn) & ~(~intersection[0] | PROMOTING_RANK(cb->turn));
  // Attacks
  br.promoted = EMPTY_PIECE;
  br.to = PAWN_ATTACKS_LEFT(b1, cb->turn) & intersection[1] & intersection[2];
  br.from = PAWN_ATTACKS_LEFT(br.to, !cb->turn);
  nodes += TraverseFn(l, cb, br);
  br.to = PAWN_ATTACKS_RIGHT(b1, cb->turn) & intersection[1] & intersection[2];
  br.from = PAWN_ATTACKS_RIGHT(br.to, !cb->turn);
  nodes += TraverseFn(l, cb, br);

  // Pushes
  b2 = SINGLE_PUSH(b1, cb->turn) & ~ALL;
  br.to = b2 & intersection[2];
  br.from = SINGLE_PUSH(br.to, !cb->turn);
  nodes += TraverseFn(l, cb, br);
  br.to = SINGLE_PUSH(b2 & ENPASSANT_RANK(cb->turn), cb->turn) & ~ALL & intersection[2];
  br.from = DOUBLE_PUSH(br.to, !cb->turn);
  nodes += TraverseFn(l, cb, br);

  return nodes;
}

inline static long enPassantBranches(LookupTable l, ChessBoard *cb, TraverseFn traverseFn, BitBoard intersection[]) {
  long nodes = 0;
  BitBoard b;
  Branch br;
  Square s;

  b = ENPASSANT(cb->enPassant) & OUR(Pawn) & intersection[0];
  while (b) {
    s = BitBoardPopLSB(&b);
    // Check that the pawn is not "pseudo-pinned"
    if (LookupTableAttacks(l, BitBoardGetLSB(OUR(King)), Rook, ALL & ~BitBoardSetBit(cb->enPassant, s)) &
        GET_RANK(BitBoardGetLSB(OUR(King))) & (THEIR(Rook) | THEIR(Queen))) continue;
    br.to = cb->enPassant;
    br.from = BitBoardSetBit(EMPTY_BOARD, s);
    nodes += traverseFn(l, cb, br);
  }

  return nodes;
}

inline static long kingBranches(LookupTable l, ChessBoard *cb, TraverseFn traverseFn, BitBoard intersection[]) {
  Branch br;
  br.to = LookupTableAttacks(l, BitBoardGetLSB(OUR(King)), King, ALL) & intersection[0] & intersection[1];
  br.from = OUR(King);
  br.promoted = EMPTY_PIECE;
  return traverseFn(l, cb, br);
}

inline static long castlingBranches(LookupTable l, ChessBoard *cb, TraverseFn traverseFn, BitBoard intersection[]) {
  BitBoard b = (cb->castling | (ALL & OCCUPANCY_MASK) | (intersection[0] & ATTACK_MASK)) & BACK_RANK(cb->turn);
  Branch br;
  br.promoted = EMPTY_PIECE;
  br.from = OUR(King);
  br.to = EMPTY_BOARD;
  if ((b & KINGSIDE) == (KINGSIDE_CASTLING & BACK_RANK(cb->turn))) br.to |= br.from << 2;
  if ((b & QUEENSIDE) == (QUEENSIDE_CASTLING & BACK_RANK(cb->turn))) br.to |= br.from >> 2;
  return traverseFn(l, cb, br);
}

inline static long pieceBranchesPinned(LookupTable l, ChessBoard *cb, TraverseFn TraverseFn, BitBoard intersection[]) {
  long nodes = 0;
  BitBoard b;
  Branch br;
  Square s;
  b = (OUR(Bishop) | OUR(Rook) | OUR(Queen)) & intersection[0];
  while (b) {
    s = BitBoardPopLSB(&b);
    br.promoted = EMPTY_PIECE;
    br.to = LookupTableAttacks(l, s, GET_TYPE(cb->squares[s]), ALL) & LookupTableGetLineOfSight(l, BitBoardGetLSB(OUR(King)), s);
    br.from = BitBoardSetBit(EMPTY_BOARD, s);
    nodes += TraverseFn(l, cb, br);
  }

  return nodes;
}

inline static long pawnBranchesPinned(LookupTable l, ChessBoard *cb, TraverseFn TraverseFn, BitBoard intersection[]) {
  long nodes = 0;
  BitBoard b1, b2, b3;
  Branch br;
  Square s;
  b1 = OUR(Pawn) & ~(~intersection[0] | PROMOTING_RANK(cb->turn));
  while (b1) {
    s = BitBoardPopLSB(&b1);
    br.from = BitBoardSetBit(EMPTY_BOARD, s);
    br.promoted = EMPTY_PIECE;
    b2 = SINGLE_PUSH(br.from, cb->turn) & ~ALL;
    b2 |= SINGLE_PUSH(b2 & ENPASSANT_RANK(cb->turn), cb->turn) & ~ALL;
    b2 |= PAWN_ATTACKS(br.from, cb->turn) & intersection[1];
    b3 = LookupTableGetLineOfSight(l, BitBoardGetLSB(OUR(King)), s); // The pin mask
    b2 &= b3;
    b2 |= ENPASSANT(br.from) & cb->enPassant & SINGLE_PUSH(b3, !cb->turn);
    br.to = b2;
    nodes += TraverseFn(l, cb, br);
  }

  return nodes;
}

inline static long promotingBranchesPinned(LookupTable l, ChessBoard *cb, TraverseFn TraverseFn, BitBoard intersection[]) {
  long nodes = 0;
  BitBoard b1, b2;
  Branch br;
  Square s;
  b1 = OUR(Pawn) & PROMOTING_RANK(cb->turn) & intersection[0];
  while (b1) {
    s = BitBoardPopLSB(&b1);
    b2 = BitBoardSetBit(EMPTY_BOARD, s);
    br.to = PAWN_ATTACKS(b2, cb->turn) & intersection[1];
    br.to &= LookupTableGetLineOfSight(l, BitBoardGetLSB(OUR(King)), s);
    br.from = b2;
    for (Type t = Knight; t <= Queen; t++) {
      br.promoted = GET_PIECE(t, cb->turn);
      nodes += TraverseFn(l, cb, br);
    }
  }

  return nodes;
}

static long treeSearch(LookupTable l, ChessBoard *cb, TraverseFn traverseFn) {
  // Base cases
  if (cb->depth == 0) return 1;
  // If we are at the node before the leaf nodes and still traversing, start counting moves
  if (cb->depth == 1 && traverseFn == traverseMoves) return treeSearch(l, cb, countMoves);
  long nodes = 0;

  // Data needed for move generation
  BitBoard us, them, pinned, checking, attacked;
  int numChecking;
  us = them = pinned = EMPTY_BOARD;
  for (Type t = Pawn; t <= Queen; t++) us |= OUR(t);
  them = ALL & ~us;
  attacked = getAttackedSquares(l, cb, them);
  checking = getCheckingPieces(l, cb, them, &pinned);
  numChecking = BitBoardCountBits(checking);

  // Traverse king branches
  nodes += kingBranches(l, cb, traverseFn, (BitBoard[]){~us, ~attacked});

  if (numChecking > 1) return nodes; // Double check, only king can move

  if (numChecking > 0) { // Single check

    BitBoard checkMask = checking | LookupTableGetSquaresBetween(l, BitBoardGetLSB(checking), BitBoardGetLSB(OUR(King)));

    // Traverse all non pinned branches
    nodes += pieceBranches(l, cb, traverseFn, (BitBoard[]){~pinned, us, checkMask});
    nodes += pawnBranches(l, cb, traverseFn, (BitBoard[]){~pinned, them, checkMask});
    nodes += promotionBranches(l, cb, traverseFn, (BitBoard[]){~pinned, them, checkMask});
    nodes += enPassantBranches(l, cb, traverseFn, (BitBoard[]){~pinned});

    return nodes;
  }

  // No check

  // Traverse all non pinned branches
  nodes += pieceBranches(l, cb, traverseFn, (BitBoard[]){~pinned, us, ~us});
  nodes += pawnBranches(l, cb, traverseFn, (BitBoard[]){~pinned, them, ~EMPTY_BOARD});
  nodes += promotionBranches(l, cb, traverseFn, (BitBoard[]){~pinned, them, ~EMPTY_BOARD});
  nodes += enPassantBranches(l, cb, traverseFn, (BitBoard[]){~pinned});

  // Traverse all pinned branches
  nodes += pieceBranchesPinned(l, cb, traverseFn, (BitBoard[]){pinned});
  nodes += pawnBranchesPinned(l, cb, traverseFn, (BitBoard[]){pinned, them}); // Includes enpassant
  nodes += promotingBranchesPinned(l, cb, traverseFn, (BitBoard[]){pinned, them});

  // Traverse castling branches
  nodes += castlingBranches(l, cb, traverseFn, (BitBoard[]){attacked});

  return nodes;
}

static long traverseMoves(LookupTable l, ChessBoard *cb, Branch br) {
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

static long printMoves(LookupTable l, ChessBoard *cb, Branch br) {
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
    printMove(u.moved, m, subTree);
    undoMove(cb, m, u);
  }

  return nodes;
}

static long countMoves(LookupTable l, ChessBoard *cb, Branch br) {
  (void) l;
  (void) cb;
  return BitBoardCountBits(br.to);
}

long ChessBoardTreeSearch(LookupTable l, ChessBoard cb, int print) {
  return (print == TRUE) ? treeSearch(l, &cb, printMoves) : treeSearch(l, &cb, traverseMoves);
}

inline static UndoData move(ChessBoard *cb, Move m) {
  int offset = m.from - m.to;

  UndoData u;
  u.moved = cb->squares[m.from];
  u.enPassant = cb->enPassant;
  u.castling = cb->castling;
  cb->castling &= ~(BitBoardSetBit(EMPTY_BOARD, m.from) | BitBoardSetBit(EMPTY_BOARD, m.to));
  u.captured = addPiece(cb, m.to, u.moved);
  addPiece(cb, m.from, EMPTY_PIECE);
  cb->enPassant = EMPTY_BOARD;

  if (GET_TYPE(u.moved) == Pawn) {
    if ((offset == 16) || (offset == -16)) { // Double push
      cb->enPassant = BitBoardSetBit(EMPTY_BOARD, m.to);
    } else if ((offset == 1) || (offset == -1)) { // Enpassant
      addPiece(cb, m.to + ((cb->turn) ? EDGE_SIZE : -EDGE_SIZE), u.moved);
      addPiece(cb, m.to, EMPTY_PIECE);
    } else if (m.promoted != EMPTY_PIECE) { // Promotion
      addPiece(cb, m.to, m.promoted);
    }
  } else if (GET_TYPE(u.moved) == King) {
    if (offset == 2) { // Queenside castling
      addPiece(cb, m.to - 2, EMPTY_PIECE);
      addPiece(cb, m.to + 1, GET_PIECE(Rook, cb->turn));
    } else if (offset == -2) { // Kingside castling
      addPiece(cb, m.to + 1, EMPTY_PIECE);
      addPiece(cb, m.to - 1, GET_PIECE(Rook, cb->turn));
    }
  }

  cb->turn = !cb->turn;
  cb->depth--;
  return u;
}

inline static void undoMove(ChessBoard *cb, Move m, UndoData u) {
  int offset = m.from - m.to;

  cb->enPassant = u.enPassant;
  cb->castling = u.castling;
  addPiece(cb, m.from, u.moved);
  addPiece(cb, m.to, u.captured);

  if (GET_TYPE(u.moved) == Pawn) {
    if ((offset == 1) || (offset == -1)) { // Enpassant
      addPiece(cb, m.to + ((cb->turn) ? -EDGE_SIZE : EDGE_SIZE), EMPTY_PIECE);
    }
  } else if (GET_TYPE(u.moved) == King) {
    if (offset == 2) { // Queenside castling
      addPiece(cb, m.to - 2, GET_PIECE(Rook, !cb->turn));
      addPiece(cb, m.to + 1, EMPTY_PIECE);
    } else if (offset == -2) { // Kingside castling
      addPiece(cb, m.to + 1, GET_PIECE(Rook, !cb->turn));
      addPiece(cb, m.to - 1, EMPTY_PIECE);
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

static void printMove(Piece moving, Move m, long nodes) {
  // Handle en passant move being encded differently
  int offset = m.from - m.to;
  if ((GET_TYPE(moving) == Pawn) && ((offset == 1) || (offset == -1))) m.to = (GET_COLOR(moving)) ? m.to + EDGE_SIZE : m.to - EDGE_SIZE;
  printf("%c%d%c%d: %ld\n", 'a' + (m.from % EDGE_SIZE), EDGE_SIZE - (m.from / EDGE_SIZE), 'a' + (m.to % EDGE_SIZE), EDGE_SIZE - (m.to / EDGE_SIZE), nodes);
}

// Return the checking pieces and simultaneously update the pinned pieces bitboard
static BitBoard getCheckingPieces(LookupTable l, ChessBoard *cb, BitBoard them, BitBoard *pinned) {
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