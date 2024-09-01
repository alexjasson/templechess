#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include "Branch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
static void addBranch(Branch *b, BitBoard to, BitBoard from, Piece moved);

Branch BranchNew(LookupTable l, ChessBoard *cb) {
  Branch b;
  b.size = 0;
  Square s;
  BitBoard us, them, pinned, checking, attacked, checkMask, moves, b1, b2, b3;
  us = OUR(Pawn) | OUR(Knight) | OUR(Bishop) | OUR(Rook) | OUR(Queen) | OUR(King);
  them = ALL & ~us;

  attacked = getAttackedSquares(l, cb, them);
  checking = getCheckingPieces(l, cb, them, &pinned);
  checkMask = ~EMPTY_BOARD;
  while (checking) {
    s = BitBoardPopLSB(&checking);
    checkMask &= (BitBoardSetBit(EMPTY_BOARD, s) | LookupTableGetSquaresBetween(l, BitBoardGetLSB(OUR(King)), s));
  }

  // King moves
  moves = LookupTableAttacks(l, BitBoardGetLSB(OUR(King)), King, EMPTY_BOARD) & ~us & ~attacked;
  b1 = (cb->castling | (attacked & ATTACK_MASK) | (ALL & OCCUPANCY_MASK)) & BACK_RANK(cb->turn);
  if ((b1 & KINGSIDE) == (KINGSIDE_CASTLING & BACK_RANK(cb->turn)) && (~checkMask == EMPTY_BOARD)) moves |= OUR(King) << 2;
  if ((b1 & QUEENSIDE) == (QUEENSIDE_CASTLING & BACK_RANK(cb->turn)) && (~checkMask == EMPTY_BOARD)) moves |= OUR(King) >> 2;
  addBranch(&b, moves, OUR(King), GET_PIECE(King, cb->turn));

  // Piece moves
  b1 = us & ~(OUR(Pawn) | OUR(King));
  while (b1) {
    s = BitBoardPopLSB(&b1);
    moves = LookupTableAttacks(l, s, GET_TYPE(cb->squares[s]), ALL) & ~us & checkMask;
    if (BitBoardSetBit(EMPTY_BOARD, s) & pinned) {
      moves &= LookupTableGetLineOfSight(l, BitBoardGetLSB(OUR(King)), s);
    }
    addBranch(&b, moves, BitBoardSetBit(EMPTY_BOARD, s), cb->squares[s]);
  }

  // Pawn moves
  b1 = OUR(Pawn);
  moves = PAWN_ATTACKS_LEFT(b1, cb->turn) & them & checkMask;
  addBranch(&b, moves, PAWN_ATTACKS_LEFT(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));
  moves = PAWN_ATTACKS_RIGHT(b1, cb->turn) & them & checkMask;
  addBranch(&b, moves, PAWN_ATTACKS_RIGHT(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));
  b2 = SINGLE_PUSH(b1, cb->turn) & ~ALL;
  moves = b2 & checkMask;
  addBranch(&b, moves, SINGLE_PUSH(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));
  moves = SINGLE_PUSH(b2 & ENPASSANT_RANK(cb->turn), cb->turn) & ~ALL & checkMask;
  addBranch(&b, moves, DOUBLE_PUSH(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));

  b1 = OUR(Pawn) & pinned;
  int i = b.size - 4;
  while (b1) {
    s = BitBoardPopLSB(&b1);
    b2 = BitBoardSetBit(EMPTY_BOARD, s); // the pinned pawn
    b3 = LookupTableGetLineOfSight(l, BitBoardGetLSB(OUR(King)), s); // pin mask

    // Remove any attacks that aren't on the pin mask from the set
    b.to[i] &= ~(PAWN_ATTACKS_LEFT(b2, cb->turn) & ~b3);
    b.from[i] = PAWN_ATTACKS_LEFT(b.to[i], (!cb->turn));
    i++;
    b.to[i] &= ~(PAWN_ATTACKS_RIGHT(b2, cb->turn) & ~b3);
    b.from[i] = PAWN_ATTACKS_RIGHT(b.to[i], (!cb->turn));
    i++;
    b.to[i] &= ~(SINGLE_PUSH(b2, cb->turn) & ~b3);
    b.from[i] = SINGLE_PUSH(b.to[i], (!cb->turn));
    i++;
    b.to[i] &= ~(DOUBLE_PUSH(b2, cb->turn) & ~b3);
    b.from[i] = DOUBLE_PUSH(b.to[i], (!cb->turn));
    i -= 3;
  }

  // Add enpassant moves
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
    addBranch(&b, b3, b2, GET_PIECE(Pawn, cb->turn));
  }

  return b;
}

int BranchCount(Branch *b) {
  int x, y, offset, nodes = 0;

  for (int i = 0; i < b->size; i++) {
    if ((b->to[i] == EMPTY_BOARD || b->from[i] == EMPTY_BOARD)) continue;
    x = BitBoardCountBits(b->to[i]);
    y = BitBoardCountBits(b->from[i]);
    offset = x - y;
    Type t = GET_TYPE(b->moved[i]);
    Color c = GET_COLOR(b->moved[i]);

    BitBoard promotion = EMPTY_BOARD;
    if (t == Pawn) promotion |= (PROMOTING_RANK(c) & b->to[i]);
    if (offset < 0) nodes++;
    nodes += BitBoardCountBits(b->to[i]) + BitBoardCountBits(promotion) * 3;
  }
  return nodes;
}

int BranchExtract(Branch *b, Move *moves) {
  Move m;
  int index = 0;
  for (int i = 0; i < b->size; i++) {
    if ((b->to[i] == EMPTY_BOARD || b->from[i] == EMPTY_BOARD)) continue;
    int x = BitBoardCountBits(b->to[i]);
    int y = BitBoardCountBits(b->from[i]);
    int offset = x - y;
    int max = (x > y) ? x : y;

    for (int j = 0; j < max; j++) {
      m.from = BitBoardPopLSB(&b->from[i]);
      m.to = BitBoardPopLSB(&b->to[i]);
      m.moved = b->moved[i];
      if (offset > 0) b->from[i] = BitBoardSetBit(EMPTY_BOARD, m.from); // Injective
      else if (offset < 0) b->to[i] = BitBoardSetBit(EMPTY_BOARD, m.to); // Surjective
      Color c = GET_COLOR(m.moved);
      Type t = GET_TYPE(m.moved);
      int promotion = (t == Pawn) && (BitBoardSetBit(EMPTY_BOARD, m.to) & PROMOTING_RANK(c));
      if (promotion) m.moved = GET_PIECE(Knight, c);

      if (promotion) {
        for (Type t = Knight; t <= Queen; t++) {
          m.moved = GET_PIECE(t, c);
          moves[index++] = m;
        }
      } else {
        moves[index++] = m;
      }
    }
  }
  return index;
}

void addBranch(Branch *b, BitBoard to, BitBoard from, Piece moved) {
  b->to[b->size] = to;
  b->from[b->size] = from;
  b->moved[b->size] = moved;
  b->size++;
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
  *pinned = EMPTY_BOARD;
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
