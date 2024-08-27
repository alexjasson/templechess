#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include "Branch.h"

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
#define PAWN_ATTACKS(b, c) ((c == White) ? BitBoardShiftNW(b) | BitBoardShiftNE(b) : BitBoardShiftSW(b) | BitBoardShiftSE(b))
#define PAWN_ATTACKS_LEFT(b, c) ((c == White) ? BitBoardShiftNW(b) : BitBoardShiftSE(b))
#define PAWN_ATTACKS_RIGHT(b, c) ((c == White) ? BitBoardShiftNE(b) : BitBoardShiftSW(b))
#define SINGLE_PUSH(b, c) ((c == White) ? BitBoardShiftN(b) : BitBoardShiftS(b))
#define DOUBLE_PUSH(b, c) ((c == White) ? BitBoardShiftN(BitBoardShiftN(b)) : BitBoardShiftS(BitBoardShiftS(b)))

#define GET_RANK(s) (SOUTH_EDGE >> (EDGE_SIZE * (EDGE_SIZE - BitBoardGetRank(s) - 1)))
#define ENPASSANT_RANK(c) (BitBoard)SOUTH_EDGE >> (EDGE_SIZE * ((c * 3) + 2)) // 6th rank for black, 3rd for white
#define PROMOTING_RANK(c) (BitBoard)((c == White) ? NORTH_EDGE : SOUTH_EDGE)
#define BACK_RANK(c) (BitBoard)((c == White) ? SOUTH_EDGE : NORTH_EDGE)

static Color getColorFromASCII(char asciiColor);
static Piece getPieceFromASCII(char asciiPiece);

static void addPiece(ChessBoard *cb, Square s, Piece replacement);
static BitBoard getAttackedSquares(LookupTable l, ChessBoard *cb, BitBoard them);
static BitBoard getCheckingPieces(LookupTable l, ChessBoard *cb, BitBoard them, BitBoard *pinned);
static long treeSearch(LookupTable l, ChessBoard *cb);
static long traverseMoves(LookupTable l, ChessBoard *cb, Branch *br);

// Assumes FEN and depth is valid
ChessBoard ChessBoardNew(char *fen, int depth) {
  ChessBoard cb;
  memset(&cb, 0, sizeof(ChessBoard));

  // Parse pieces and squares
  for (Square s = 0; s < BOARD_SIZE && *fen; fen++) {
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
    cb.enPassant = rank * EDGE_SIZE + file;
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
  BitBoard us, them, pinned, checking, attacked, checkMask, b1, b2, b3;
  Square s;
  Branch curr = BranchNew();
  us = them = pinned = EMPTY_BOARD;
  for (Type t = Pawn; t <= Queen; t++) us |= OUR(t);
  them = ALL & ~us;

  BitBoard moves;

  // King branches
  // br[0].to = LookupTableAttacks(l, BitBoardGetLSB(OUR(King)), King, EMPTY_BOARD) & ~us;
  // br[0].from = OUR(King);
  // brSize++;
  moves = LookupTableAttacks(l, BitBoardGetLSB(OUR(King)), King, EMPTY_BOARD) & ~us;
  BranchAdd(&curr, moves, OUR(King));

  // Piece branches
  b1 = us & ~(OUR(Pawn) | OUR(King));
  while (b1) {
    s = BitBoardPopLSB(&b1);
    // br[brSize].to = LookupTableAttacks(l, s, GET_TYPE(cb->squares[s]), ALL) & ~us;
    // br[brSize].from = BitBoardSetBit(EMPTY_BOARD, s);
    // brSize++;
    moves = LookupTableAttacks(l, s, GET_TYPE(cb->squares[s]), ALL) & ~us;
    BranchAdd(&curr, moves, BitBoardSetBit(EMPTY_BOARD, s));
  }

  // Pawn branches
  b1 = OUR(Pawn);
  // br[brSize].to = PAWN_ATTACKS_LEFT(b1, cb->turn) & them;
  // br[brSize].from = PAWN_ATTACKS_LEFT(br[brSize].to, (!cb->turn));
  // brSize++;
  moves = PAWN_ATTACKS_LEFT(b1, cb->turn) & them;
  BranchAdd(&curr, moves, PAWN_ATTACKS_LEFT(moves, (!cb->turn)));
  // br[brSize].to = PAWN_ATTACKS_RIGHT(b1, cb->turn) & them;
  // br[brSize].from = PAWN_ATTACKS_RIGHT(br[brSize].to, (!cb->turn));
  // brSize++;
  moves = PAWN_ATTACKS_RIGHT(b1, cb->turn) & them;
  BranchAdd(&curr, moves, PAWN_ATTACKS_RIGHT(moves, (!cb->turn)));



  attacked = getAttackedSquares(l, cb, them);
  checking = getCheckingPieces(l, cb, them, &pinned);
  checkMask = ~EMPTY_BOARD;
  while (checking) {
    s = BitBoardPopLSB(&checking);
    checkMask &= (BitBoardSetBit(EMPTY_BOARD, s) | LookupTableGetSquaresBetween(l, BitBoardGetLSB(OUR(King)), s));
  }

  // Prune king branch and add castling - attacks
  curr.to[0] &= ~attacked;
  b1 = (cb->castling | (attacked & ATTACK_MASK) | (ALL & OCCUPANCY_MASK)) & BACK_RANK(cb->turn);
  if ((b1 & KINGSIDE) == (KINGSIDE_CASTLING & BACK_RANK(cb->turn)) && (~checkMask == EMPTY_BOARD)) curr.to[0] |= OUR(King) << 2;
  if ((b1 & QUEENSIDE) == (QUEENSIDE_CASTLING & BACK_RANK(cb->turn)) && (~checkMask == EMPTY_BOARD)) curr.to[0] |= OUR(King) >> 2;

  // Prune piece branches - checks and pins
  int i = 1;
  b1 = us & ~(OUR(Pawn) | OUR(King));
  while (b1) {
    s = BitBoardPopLSB(&b1);
    if (BitBoardSetBit(EMPTY_BOARD, s) & pinned) {
      curr.to[i] &= LookupTableGetLineOfSight(l, BitBoardGetLSB(OUR(King)), s);
    }
    curr.to[i++] &= checkMask;
  }

  // Prune pawn branches and add pushes - checks and pins
  b1 = OUR(Pawn);
  curr.to[i] &= checkMask;
  curr.from[i] = PAWN_ATTACKS_LEFT(curr.to[i], (!cb->turn));
  i++;
  curr.to[i] &= checkMask;
  curr.from[i] = PAWN_ATTACKS_RIGHT(curr.to[i], (!cb->turn));
  i++;
  b2 = SINGLE_PUSH(b1, cb->turn) & ~ALL;
  // br[i].to = b2 & checkMask;
  // br[i].from = SINGLE_PUSH(br[i].to, (!cb->turn));
  // brSize++;
  moves = b2 & checkMask;
  BranchAdd(&curr, moves, SINGLE_PUSH(moves, (!cb->turn)));
  i++;
  // br[i].to = SINGLE_PUSH(b2 & ENPASSANT_RANK(cb->turn), cb->turn) & ~ALL & checkMask;
  // br[i].from = DOUBLE_PUSH(br[i].to, (!cb->turn));
  // brSize++;
  moves = SINGLE_PUSH(b2 & ENPASSANT_RANK(cb->turn), cb->turn) & ~ALL & checkMask;
  BranchAdd(&curr, moves, DOUBLE_PUSH(moves, (!cb->turn)));


  b1 = OUR(Pawn) & pinned;
  while (b1) {
    s = BitBoardPopLSB(&b1);
    b2 = BitBoardSetBit(EMPTY_BOARD, s); // the pinned pawn
    b3 = LookupTableGetLineOfSight(l, BitBoardGetLSB(OUR(King)), s); // pin mask

    // Remove any attacks that aren't on the pin mask from the set
    i -= 3;
    curr.to[i] &= ~(PAWN_ATTACKS_LEFT(b2, cb->turn) & ~b3);
    curr.from[i] = PAWN_ATTACKS_LEFT(curr.to[i], (!cb->turn));
    i++;
    curr.to[i] &= ~(PAWN_ATTACKS_RIGHT(b2, cb->turn) & ~b3);
    curr.from[i] = PAWN_ATTACKS_RIGHT(curr.to[i], (!cb->turn));
    i++;
    curr.to[i] &= ~(SINGLE_PUSH(b2, cb->turn) & ~b3);
    curr.from[i] = SINGLE_PUSH(curr.to[i], (!cb->turn));
    i++;
    curr.to[i] &= ~(DOUBLE_PUSH(b2, cb->turn) & ~b3);
    curr.from[i] = DOUBLE_PUSH(curr.to[i], (!cb->turn));
  }

  // Add enpassant branch and prune it - pin squares
  if (cb->enPassant != EMPTY_SQUARE) {
    b1 = PAWN_ATTACKS(BitBoardSetBit(EMPTY_BOARD, cb->enPassant), (!cb->turn)) & OUR(Pawn);
    b2 = EMPTY_BOARD;
    b3 = BitBoardSetBit(EMPTY_BOARD, cb->enPassant); // Enpassant square in bitboard form
    while (b1) {
      s = BitBoardPopLSB(&b1);

      // Check that the pawn is not "pseudo-pinned"
      if (LookupTableAttacks(l, BitBoardGetLSB(OUR(King)), Rook, ALL & ~BitBoardSetBit(SINGLE_PUSH(b3, (!cb->turn)), s)) &
          GET_RANK(BitBoardGetLSB(OUR(King))) & (THEIR(Rook) | THEIR(Queen))) continue;
      b2 |= BitBoardSetBit(EMPTY_BOARD, s);
      if (b2 & pinned) b2 &= LookupTableGetLineOfSight(l, BitBoardGetLSB(OUR(King)), cb->enPassant);
    }
    // br[brSize].to = b3;
    // br[brSize].from = b2;
    // brSize++;
    BranchAdd(&curr, b3, b2);
  }

  return traverseMoves(l, cb, &curr);
}

static long traverseMoves(LookupTable l, ChessBoard *cb, Branch *br) {
  long nodes = 0;
  int a, b;
  ChessBoard new;
  Move m;

  if (cb->depth == 1) {
    for (int i = 0; i < br->size; i++) {
      if (BranchIsEmpty(br, i)) continue;
      a = BitBoardCountBits(br->to[i]);
      b = BitBoardCountBits(br->from[i]);
      int offset = a - b;

      BitBoard promotion = EMPTY_BOARD;
      if (GET_TYPE(cb->squares[BitBoardGetLSB(br->from[i])]) == Pawn) promotion |= (PROMOTING_RANK(cb->turn) & br->to[i]);
      if (offset < 0) nodes++;
      nodes += BitBoardCountBits(br->to[i]) + BitBoardCountBits(promotion) * 3;
    }
    return nodes;
  }

  for (int i = 0; i < br->size; i++) {
    if (BranchIsEmpty(br, i)) continue;
    a = BitBoardCountBits(br->to[i]);
    b = BitBoardCountBits(br->from[i]);
    int offset = a - b;
    int max = (a > b) ? a : b;

    for (int j = 0; j < max; j++) {
      m.from = BitBoardPopLSB(&br->from[i]); // COuld be an issue here
      m.to = BitBoardPopLSB(&br->to[i]);
      m.moved = GET_TYPE(cb->squares[m.from]);
      if (offset > 0) br->from[i] = BitBoardSetBit(EMPTY_BOARD, m.from); // Injective
      else if (offset < 0) br->to[i] = BitBoardSetBit(EMPTY_BOARD, m.to); // Surjective
      int promotion = ((m.moved == Pawn) && (BitBoardSetBit(EMPTY_BOARD, m.to) & PROMOTING_RANK(cb->turn)));
      if (promotion) m.moved = Knight;

      Move:
      ChessBoardPlayMove(&new, cb, m);
      nodes += treeSearch(l, &new); // Continue traversing

      if (promotion && m.moved < Queen) {
        m.moved++;
        goto Move;
      }
    }
  }

  return nodes;
}

// Given an old board and a new board, copy the old board and play the move on the new board
void ChessBoardPlayMove(ChessBoard *new, ChessBoard *old, Move m) {
  memcpy(new, old, sizeof(ChessBoard));
  int offset = m.from - m.to;
  new->enPassant = EMPTY_SQUARE;
  new->castling &= ~(BitBoardSetBit(EMPTY_BOARD, m.from) | BitBoardSetBit(EMPTY_BOARD, m.to));
  addPiece(new, m.to, GET_PIECE(m.moved, new->turn));
  addPiece(new, m.from, EMPTY_PIECE);

  if (m.moved == Pawn) {
    if ((offset == 16) || (offset == -16)) { // Double push
      new->enPassant = m.from - (offset / 2);
    } else if (m.to == old->enPassant) { // Enpassant
      addPiece(new, m.to + (new->turn ? -8: 8), EMPTY_PIECE);
    }
  } else if (m.moved == King) {
    if (offset == 2) { // Queenside castling
      addPiece(new, m.to - 2, EMPTY_PIECE);
      addPiece(new, m.to + 1, GET_PIECE(Rook, new->turn));
    } else if (offset == -2) { // Kingside castling
      addPiece(new, m.to + 1, EMPTY_PIECE);
      addPiece(new, m.to - 1, GET_PIECE(Rook, new->turn));
    }
  }

  new->turn = !new->turn;
  new->depth--;
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
