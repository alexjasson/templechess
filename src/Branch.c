#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include "Branch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#define PAWN_ATTACKS(b, c) ((c == White) ? BitBoardShiftNW(b) | BitBoardShiftNE(b) : BitBoardShiftSW(b) | BitBoardShiftSE(b))
#define PAWN_ATTACKS_LEFT(b, c) ((c == White) ? BitBoardShiftNW(b) : BitBoardShiftSE(b))
#define PAWN_ATTACKS_RIGHT(b, c) ((c == White) ? BitBoardShiftNE(b) : BitBoardShiftSW(b))
#define SINGLE_PUSH(b, c) ((c == White) ? BitBoardShiftN(b) : BitBoardShiftS(b))
#define DOUBLE_PUSH(b, c) ((c == White) ? BitBoardShiftN(BitBoardShiftN(b)) : BitBoardShiftS(BitBoardShiftS(b)))

// Given a square, returns a bitboard representing the rank of that square
#define GET_RANK(s) (SOUTH_EDGE >> (EDGE_SIZE * (EDGE_SIZE - BitBoardGetRank(s) - 1)))
#define ENPASSANT_RANK(c) (BitBoard)SOUTH_EDGE >> (EDGE_SIZE * ((c * 3) + 2)) // 6th rank for black, 3rd for white
#define PROMOTING_RANK(c) (BitBoard)((c == White) ? NORTH_EDGE : SOUTH_EDGE)
#define BACK_RANK(c) (BitBoard)((c == White) ? SOUTH_EDGE : NORTH_EDGE)

static BitBoard getAttackedSquares(LookupTable l, ChessBoard *cb, BitBoard them);
static BitBoard getCheckingPieces(LookupTable l, ChessBoard *cb, BitBoard them, BitBoard *pinned);
static long treeSearch(LookupTable l, ChessBoard *cb);

Branch BranchNew() {
  Branch b;
  memset(&b, 0, sizeof(Branch));
  return b;
}

void BranchAdd(Branch *b, BitBoard to, BitBoard from) {
  b->to[b->size] = to;
  b->from[b->size] = from;
  b->size++;
}

// Returns whether an element in a branch is empty
int BranchIsEmpty(Branch *b, int index) {
  return b->to[index] == EMPTY_BOARD || b->from[index] == EMPTY_BOARD;
}

long BranchTreeSearch(ChessBoard *cb) {
  long nodes = 0;
  LookupTable l = LookupTableNew();
  nodes = treeSearch(l, cb);
  LookupTableFree(l);
  return nodes;
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
  moves = LookupTableAttacks(l, BitBoardGetLSB(OUR(King)), King, EMPTY_BOARD) & ~us;
  BranchAdd(&curr, moves, OUR(King));

  // Piece branches
  b1 = us & ~(OUR(Pawn) | OUR(King));
  while (b1) {
    s = BitBoardPopLSB(&b1);
    moves = LookupTableAttacks(l, s, GET_TYPE(cb->squares[s]), ALL) & ~us;
    BranchAdd(&curr, moves, BitBoardSetBit(EMPTY_BOARD, s));
  }

  // Pawn branches
  b1 = OUR(Pawn);
  moves = PAWN_ATTACKS_LEFT(b1, cb->turn) & them;
  BranchAdd(&curr, moves, PAWN_ATTACKS_LEFT(moves, (!cb->turn)));
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
  b2 = SINGLE_PUSH(b1, cb->turn) & ~ALL;
  moves = b2 & checkMask;
  BranchAdd(&curr, moves, SINGLE_PUSH(moves, (!cb->turn)));
  moves = SINGLE_PUSH(b2 & ENPASSANT_RANK(cb->turn), cb->turn) & ~ALL & checkMask;
  BranchAdd(&curr, moves, DOUBLE_PUSH(moves, (!cb->turn)));
  i--;

  b1 = OUR(Pawn) & pinned;
  while (b1) {
    s = BitBoardPopLSB(&b1);
    b2 = BitBoardSetBit(EMPTY_BOARD, s); // the pinned pawn
    b3 = LookupTableGetLineOfSight(l, BitBoardGetLSB(OUR(King)), s); // pin mask

    // Remove any attacks that aren't on the pin mask from the set
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
    i -= 3;
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
    BranchAdd(&curr, b3, b2);
  }

  // ------- BEGIN RECURSIVE PART ----------
  long nodes = 0;
  int a, b;
  ChessBoard new;
  Move m;

  if (cb->depth == 1) {
    for (int i = 0; i < curr.size; i++) {
      if (BranchIsEmpty(&curr, i)) continue;
      a = BitBoardCountBits(curr.to[i]);
      b = BitBoardCountBits(curr.from[i]);
      int offset = a - b;

      BitBoard promotion = EMPTY_BOARD;
      if (GET_TYPE(cb->squares[BitBoardGetLSB(curr.from[i])]) == Pawn) promotion |= (PROMOTING_RANK(cb->turn) & curr.to[i]);
      if (offset < 0) nodes++;
      nodes += BitBoardCountBits(curr.to[i]) + BitBoardCountBits(promotion) * 3;
    }
    return nodes;
  }

  for (int i = 0; i < curr.size; i++) {
    if (BranchIsEmpty(&curr, i)) continue;
    a = BitBoardCountBits(curr.to[i]);
    b = BitBoardCountBits(curr.from[i]);
    int offset = a - b;
    int max = (a > b) ? a : b;

    for (int j = 0; j < max; j++) {
      m.from = BitBoardPopLSB(&curr.from[i]);
      m.to = BitBoardPopLSB(&curr.to[i]);
      m.moved = GET_TYPE(cb->squares[m.from]);
      if (offset > 0) curr.from[i] = BitBoardSetBit(EMPTY_BOARD, m.from); // Injective
      else if (offset < 0) curr.to[i] = BitBoardSetBit(EMPTY_BOARD, m.to); // Surjective
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