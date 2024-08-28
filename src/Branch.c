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
static long treeSearch(LookupTable l, ChessBoard *cb, Branch *curr);

Branch BranchNew() {
  Branch b;
  memset(&b, 0, sizeof(Branch));
  return b;
}

void BranchAdd(Branch *b, BitBoard to, BitBoard from, Type moved) {
  b->to[b->size] = to;
  b->from[b->size] = from;
  b->moved[b->size] = moved;
  b->size++;
}

// Returns whether an element in a branch is empty
int BranchIsEmpty(Branch *b, int index) {
  return b->to[index] == EMPTY_BOARD || b->from[index] == EMPTY_BOARD;
}

int BranchCount(Branch *b, Color c) {
  int x, y, offset, nodes = 0;

  for (int i = 0; i < b->size; i++) {
    if (BranchIsEmpty(b, i)) continue;
    x = BitBoardCountBits(b->to[i]);
    y = BitBoardCountBits(b->from[i]);
    offset = x - y;

    BitBoard promotion = EMPTY_BOARD;
    if (b->moved[i] == Pawn) promotion |= (PROMOTING_RANK(c) & b->to[i]);
    if (offset < 0) nodes++;
    nodes += BitBoardCountBits(b->to[i]) + BitBoardCountBits(promotion) * 3;
  }
  return nodes;
}

long BranchTreeSearch(ChessBoard *cb) {
  long nodes = 0;
  Branch curr = BranchNew();
  LookupTable l = LookupTableNew();
  nodes = treeSearch(l, cb, &curr);
  LookupTableFree(l);
  return nodes;
}





static long treeSearch(LookupTable l, ChessBoard *cb, Branch *curr) {
  if (cb->depth == 0) return 1; // Base case
  memset(curr, 0, sizeof(Branch));

  // Data needed for move generation
  Square s;
  Type t;
  BitBoard us, them, pinned, checking, attacked, checkMask, moves, b1, b2, b3;
  us = OUR(Pawn) | OUR(Knight) | OUR(Bishop) | OUR(Rook) | OUR(Queen) | OUR(King);
  them = ALL & ~us;

  // King branches
  moves = LookupTableAttacks(l, BitBoardGetLSB(OUR(King)), King, EMPTY_BOARD) & ~us;
  BranchAdd(curr, moves, OUR(King), King);

  // Piece branches
  b1 = us & ~(OUR(Pawn) | OUR(King));
  while (b1) {
    s = BitBoardPopLSB(&b1);
    t = GET_TYPE(cb->squares[s]);
    moves = LookupTableAttacks(l, s, GET_TYPE(cb->squares[s]), ALL) & ~us;
    BranchAdd(curr, moves, BitBoardSetBit(EMPTY_BOARD, s), t);
  }

  // Pawn branches
  b1 = OUR(Pawn);
  moves = PAWN_ATTACKS_LEFT(b1, cb->turn) & them;
  BranchAdd(curr, moves, PAWN_ATTACKS_LEFT(moves, (!cb->turn)), Pawn);
  moves = PAWN_ATTACKS_RIGHT(b1, cb->turn) & them;
  BranchAdd(curr, moves, PAWN_ATTACKS_RIGHT(moves, (!cb->turn)), Pawn);



  attacked = getAttackedSquares(l, cb, them);
  checking = getCheckingPieces(l, cb, them, &pinned);
  checkMask = ~EMPTY_BOARD;
  while (checking) {
    s = BitBoardPopLSB(&checking);
    checkMask &= (BitBoardSetBit(EMPTY_BOARD, s) | LookupTableGetSquaresBetween(l, BitBoardGetLSB(OUR(King)), s));
  }

  // Prune king branch and add castling - attacks
  curr->to[0] &= ~attacked;
  b1 = (cb->castling | (attacked & ATTACK_MASK) | (ALL & OCCUPANCY_MASK)) & BACK_RANK(cb->turn);
  if ((b1 & KINGSIDE) == (KINGSIDE_CASTLING & BACK_RANK(cb->turn)) && (~checkMask == EMPTY_BOARD)) curr->to[0] |= OUR(King) << 2;
  if ((b1 & QUEENSIDE) == (QUEENSIDE_CASTLING & BACK_RANK(cb->turn)) && (~checkMask == EMPTY_BOARD)) curr->to[0] |= OUR(King) >> 2;

  // Prune piece branches - checks and pins
  int i = 1;
  b1 = us & ~(OUR(Pawn) | OUR(King));
  while (b1) {
    s = BitBoardPopLSB(&b1);
    if (BitBoardSetBit(EMPTY_BOARD, s) & pinned) {
      curr->to[i] &= LookupTableGetLineOfSight(l, BitBoardGetLSB(OUR(King)), s);
    }
    curr->to[i++] &= checkMask;
  }

  // Prune pawn branches and add pushes - checks and pins
  b1 = OUR(Pawn);
  curr->to[i] &= checkMask;
  curr->from[i] = PAWN_ATTACKS_LEFT(curr->to[i], (!cb->turn));
  i++;
  curr->to[i] &= checkMask;
  curr->from[i] = PAWN_ATTACKS_RIGHT(curr->to[i], (!cb->turn));
  b2 = SINGLE_PUSH(b1, cb->turn) & ~ALL;
  moves = b2 & checkMask;
  BranchAdd(curr, moves, SINGLE_PUSH(moves, (!cb->turn)), Pawn);
  moves = SINGLE_PUSH(b2 & ENPASSANT_RANK(cb->turn), cb->turn) & ~ALL & checkMask;
  BranchAdd(curr, moves, DOUBLE_PUSH(moves, (!cb->turn)), Pawn);
  i--;

  b1 = OUR(Pawn) & pinned;
  while (b1) {
    s = BitBoardPopLSB(&b1);
    b2 = BitBoardSetBit(EMPTY_BOARD, s); // the pinned pawn
    b3 = LookupTableGetLineOfSight(l, BitBoardGetLSB(OUR(King)), s); // pin mask

    // Remove any attacks that aren't on the pin mask from the set
    curr->to[i] &= ~(PAWN_ATTACKS_LEFT(b2, cb->turn) & ~b3);
    curr->from[i] = PAWN_ATTACKS_LEFT(curr->to[i], (!cb->turn));
    i++;
    curr->to[i] &= ~(PAWN_ATTACKS_RIGHT(b2, cb->turn) & ~b3);
    curr->from[i] = PAWN_ATTACKS_RIGHT(curr->to[i], (!cb->turn));
    i++;
    curr->to[i] &= ~(SINGLE_PUSH(b2, cb->turn) & ~b3);
    curr->from[i] = SINGLE_PUSH(curr->to[i], (!cb->turn));
    i++;
    curr->to[i] &= ~(DOUBLE_PUSH(b2, cb->turn) & ~b3);
    curr->from[i] = DOUBLE_PUSH(curr->to[i], (!cb->turn));
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
    BranchAdd(curr, b3, b2, Pawn);
  }

  // ------- BEGIN RECURSIVE PART ----------
  long nodes = 0;
  ChessBoard new;
  Move moveSet[218];

  if (cb->depth == 1) return BranchCount(curr, cb->turn);

  int size = BranchExtract(curr, moveSet, cb->turn);
  for (int i = 0; i < size; i++) {
    Move m = moveSet[i];
    ChessBoardPlayMove(&new, cb, m);
    nodes += treeSearch(l, &new, curr);
  }

  return nodes;
}

int BranchExtract(Branch *b, Move *moveSet, Color c) {
  Move m;
  int index = 0;
  for (int i = 0; i < b->size; i++) {
    if (BranchIsEmpty(b, i)) continue;
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
      int promotion = ((m.moved == Pawn) && (BitBoardSetBit(EMPTY_BOARD, m.to) & PROMOTING_RANK(c)));
      if (promotion) m.moved = Knight;

      Move:
      moveSet[index++] = m;

      if (promotion && m.moved < Queen) {
        m.moved++;
        goto Move;
      }
    }
  }
  memset(b, 0, sizeof(Branch));
  return index;
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