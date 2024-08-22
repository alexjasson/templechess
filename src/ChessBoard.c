#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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
#define PROMOTING_RANK(c) (BitBoard)((c == White) ? NORTH_EDGE : SOUTH_EDGE)
#define BACK_RANK(c) (BitBoard)((c == White) ? SOUTH_EDGE : NORTH_EDGE)

typedef struct {
  BitBoard to;
  BitBoard from;
} Branch;

typedef struct {
    Square to;
    Square from;
    Type moved;
} Move;

static Color getColorFromASCII(char asciiColor);
static Piece getPieceFromASCII(char asciiPiece);

static void addPiece(ChessBoard *cb, Square s, Piece replacement);
static void move(ChessBoard *cb, Move m);
static BitBoard getAttackedSquares(LookupTable l, ChessBoard *cb, BitBoard them);
static BitBoard getCheckingPieces(LookupTable l, ChessBoard *cb, BitBoard them, BitBoard *pinned);
static long treeSearch(LookupTable l, ChessBoard *cb);
static long traverseMoves(LookupTable l, ChessBoard *cb, Branch *br, int brSize);

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
    cb.enPassant = SINGLE_PUSH(BitBoardSetBit(EMPTY_BOARD, rank * EDGE_SIZE + file), (!cb.turn));
  }

  return cb;
}

// Assumes that asciiPiece is valid
static Piece getPieceFromASCII(char asciiPiece) {
  if (asciiPiece == '-') return EMPTY_PIECE;
  char *pieces = "PKNBRQ";
  Type t = (Type)(strchr(pieces, toupper(asciiPiece)) - pieces);
  Color c = isupper(asciiPiece) ? White : Black;
  return GET_PIECE(t, c);
}

static Color getColorFromASCII(char asciiColor) {
  return (asciiColor == 'w') ? White : Black;
}

static long treeSearch(LookupTable l, ChessBoard *cb) {
  if (cb->depth == 0) return 1; // Base case

  // Data needed for move generation
  BitBoard us, them, pinned, checking, attacked, moveMask, b1, b2;
  Square s;
  Branch br[32];
  int brSize = 1;
  int numChecking;
  us = them = pinned = EMPTY_BOARD;
  for (Type t = Pawn; t <= Queen; t++) us |= OUR(t);
  them = ALL & ~us;
  attacked = getAttackedSquares(l, cb, them);
  checking = getCheckingPieces(l, cb, them, &pinned);
  numChecking = BitBoardCountBits(checking);

  // Traverse king branches
  br[0].to = LookupTableAttacks(l, BitBoardGetLSB(OUR(King)), King, ALL) & ~us & ~attacked;
  br[0].from = OUR(King);

  if (numChecking == 2) {
    return traverseMoves(l, cb, br, brSize);
  } else if (numChecking == 1) {
    moveMask = checking | LookupTableGetSquaresBetween(l, BitBoardGetLSB(checking), BitBoardGetLSB(OUR(King)));
  } else {
    moveMask = ~us;
    // Traverse castling branches
    b1 = (cb->castling | (ALL & OCCUPANCY_MASK) | (attacked & ATTACK_MASK)) & BACK_RANK(cb->turn);
    if ((b1 & KINGSIDE) == (KINGSIDE_CASTLING & BACK_RANK(cb->turn))) br[0].to |= OUR(King) << 2;
    if ((b1 & QUEENSIDE) == (QUEENSIDE_CASTLING & BACK_RANK(cb->turn))) br[0].to |= OUR(King) >> 2;
  }

  // Traverse piece branches
  b1 = us & ~(OUR(Pawn) | OUR(King));
  while (b1) {
    s = BitBoardPopLSB(&b1);
    br[brSize].to = LookupTableAttacks(l, s, GET_TYPE(cb->squares[s]), ALL) & moveMask;
    br[brSize].from = BitBoardSetBit(EMPTY_BOARD, s);
    if (br[brSize].from & pinned) br[brSize].to &= LookupTableGetLineOfSight(l, BitBoardGetLSB(OUR(King)), s);
    brSize++;
  }

  // Non pinned pawn branches
  b1 = OUR(Pawn) & ~pinned;
  br[brSize].to = PAWN_ATTACKS_LEFT(b1, cb->turn) & them & moveMask;
  br[brSize].from = PAWN_ATTACKS_LEFT(br[brSize].to, (!cb->turn));
  brSize++;
  br[brSize].to = PAWN_ATTACKS_RIGHT(b1, cb->turn) & them & moveMask;
  br[brSize].from = PAWN_ATTACKS_RIGHT(br[brSize].to, (!cb->turn));
  brSize++;
  b2 = SINGLE_PUSH(b1, cb->turn) & ~ALL;
  br[brSize].to = b2 & moveMask;
  br[brSize].from = SINGLE_PUSH(br[brSize].to, (!cb->turn));
  brSize++;
  br[brSize].to = SINGLE_PUSH(b2 & ENPASSANT_RANK(cb->turn), cb->turn) & ~ALL & moveMask;
  br[brSize].from = DOUBLE_PUSH(br[brSize].to, (!cb->turn));
  brSize++;

  // Rest of pawn branches - pinned
  b1 = OUR(Pawn) & ~b1;
  while (b1) {
    s = BitBoardPopLSB(&b1);
    br[brSize].from = BitBoardSetBit(EMPTY_BOARD, s);
    br[brSize].to = SINGLE_PUSH(br[brSize].from, cb->turn) & ~ALL;
    br[brSize].to |= SINGLE_PUSH(br[brSize].to & ENPASSANT_RANK(cb->turn), cb->turn) & ~ALL;
    br[brSize].to |= PAWN_ATTACKS(br[brSize].from, cb->turn) & them;
    if (br[brSize].from & pinned) br[brSize].to &= LookupTableGetLineOfSight(l, BitBoardGetLSB(OUR(King)), s);
    br[brSize].to &= moveMask;
    brSize++;
  }

  // Enpassant pawn branches - make this only one branch
  b1 = ENPASSANT(cb->enPassant) & OUR(Pawn);
  while (b1) {
    s = BitBoardPopLSB(&b1);
    // Check that the pawn is not "pseudo-pinned"
    if (LookupTableAttacks(l, BitBoardGetLSB(OUR(King)), Rook, ALL & ~BitBoardSetBit(cb->enPassant, s)) &
        GET_RANK(BitBoardGetLSB(OUR(King))) & (THEIR(Rook) | THEIR(Queen))) continue;
    br[brSize].to = cb->enPassant;
    br[brSize].from = BitBoardSetBit(EMPTY_BOARD, s);
    // If it's pinned, intersect with pin mask
    if (br[brSize].from & pinned) {
      br[brSize].to &= SINGLE_PUSH(LookupTableGetLineOfSight(l, BitBoardGetLSB(OUR(King)), s), (!cb->turn));
    }
    brSize++;
  }

  return traverseMoves(l, cb, br, brSize);
}

static long traverseMoves(LookupTable l, ChessBoard *cb, Branch *br, int brSize) {
  long nodes = 0;
  ChessBoard new;
  Move m;

  // Injective king/piece branches
  for (int i = 0; i < brSize; i++) {
    if (br[i].from == EMPTY_BOARD) continue;
    if (cb->depth == 1) {
      BitBoard promotion = EMPTY_BOARD;
      if (GET_TYPE(cb->squares[BitBoardGetLSB(br[i].from)]) == Pawn) promotion |= (PROMOTING_RANK(cb->turn) & br[i].to);
      nodes += BitBoardCountBits(br[i].to) + BitBoardCountBits(promotion) * 3;
      continue;
    }

    int oneToOne = BitBoardCountBits(br[i].from) - 1; // If non-zero, then one-to-one mapping

    m.from = BitBoardGetLSB(br[i].from);
    while (br[i].to) {
      if (oneToOne) m.from = BitBoardPopLSB(&br[i].from);
      m.to = BitBoardPopLSB(&br[i].to);
      m.moved = GET_TYPE(cb->squares[m.from]);
      int promotion = ((m.moved == Pawn) && (BitBoardSetBit(EMPTY_BOARD, m.to) & PROMOTING_RANK(cb->turn)));
      if (promotion) m.moved = Knight;

      Move:
      memcpy(&new, cb, sizeof(ChessBoard));
      move(&new, m);
      nodes += treeSearch(l, &new); // Continue traversing

      if (promotion && m.moved < Queen) {
        m.moved++;
        goto Move;
      }
    }
  }

  // Bijective pawn branches

  // handle the surjective enpassant branch separately
  // ...

  return nodes;
}

static void move(ChessBoard *cb, Move m) {
  int offset = m.from - m.to;

  cb->enPassant = EMPTY_BOARD;
  cb->castling &= ~(BitBoardSetBit(EMPTY_BOARD, m.from) | BitBoardSetBit(EMPTY_BOARD, m.to));
  addPiece(cb, m.to, GET_PIECE(m.moved, cb->turn));
  addPiece(cb, m.from, EMPTY_PIECE);

  if (m.moved == Pawn) {
    if ((offset == 16) || (offset == -16)) { // Double push
      cb->enPassant = BitBoardSetBit(EMPTY_BOARD, m.to);
    } else if ((offset == 1) || (offset == -1)) { // Enpassant
      addPiece(cb, m.to + ((cb->turn) ? EDGE_SIZE : -EDGE_SIZE), GET_PIECE(m.moved, cb->turn));
      addPiece(cb, m.to, EMPTY_PIECE);
    }
  } else if (m.moved == King) {
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
}

// Adds a piece to a chessboard
static void addPiece(ChessBoard *cb, Square s, Piece replacement) {
  BitBoard b = BitBoardSetBit(EMPTY_BOARD, s);
  Piece captured = cb->squares[s];
  cb->squares[s] = replacement;
  cb->pieces[replacement] |= b;
  cb->pieces[captured] &= ~b;
}

long ChessBoardTreeSearch(ChessBoard cb) {
  LookupTable l = LookupTableNew();
  long nodes = treeSearch(l, &cb);
  LookupTableFree(l);
  return nodes;
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

  attacked = PAWN_ATTACKS(THEIR(Pawn), (!cb->turn));
  b = them & ~THEIR(Pawn);
  while (b) {
    Square s = BitBoardPopLSB(&b);
    attacked |= LookupTableAttacks(l, s, GET_TYPE(cb->squares[s]), occupancies);
  }
  return attacked;
}
