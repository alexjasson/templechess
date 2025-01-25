#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include "Branch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OUR(t) (cb->pieces[GET_PIECE(t, cb->turn)])                                     // Bitboard representing our pieces of type t
#define THEIR(t) (cb->pieces[GET_PIECE(t, !cb->turn)])                                  // Bitboard representing their pieces of type t
#define ALL (~cb->pieces[EMPTY_PIECE])                                                  // Bitboard of all the pieces
#define US (OUR(Pawn) | OUR(Knight) | OUR(Bishop) | OUR(Rook) | OUR(Queen) | OUR(King)) // Bitboard of all our pieces
#define THEM (ALL & ~US)                                                                // Bitboard of all their pieces

#define GET_RANK(s) (SOUTH_EDGE >> (EDGE_SIZE * (EDGE_SIZE - BitBoardGetRank(s) - 1))) // BitBoard representing the rank of a specific square - should probably be in BitBoard.h
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

// Should be static in future
Branch BranchNew(BitBoard to, BitBoard from, Piece moved)
{
  Branch b;
  b.to = to;
  b.from = from;
  b.moved = moved;
  return b;
}

BranchSet BranchSetNew()
{
  BranchSet bs;
  bs.start = 0;
  bs.end = 0;
  bs.branches[0].to = EMPTY_BOARD;
  bs.branches[0].from = EMPTY_BOARD;
  bs.branches[0].moved = EMPTY_PIECE;
  return bs;
}

void BranchFill(LookupTable l, ChessBoard *cb, BranchSet *bs)
{
  // Initialize branch set
  memset(bs, 0, sizeof(BranchSet));
  bs->prev.moved = EMPTY_PIECE;
  bs->prev.from = EMPTY_SQUARE;
  bs->prev.to = EMPTY_SQUARE;

  Square s;
  BitBoard pinned, checking, attacked, checkMask, moves, b1, b2, b3;

  attacked = ChessBoardAttacked(l, cb);
  checking = ChessBoardChecking(l, cb);
  pinned = ChessBoardPinned(l, cb);
  checkMask = ~EMPTY_BOARD;
  while (checking)
  {
    s = BitBoardPopLSB(&checking);
    checkMask &= (BitBoardSetBit(EMPTY_BOARD, s) | LookupTableGetSquaresBetween(l, BitBoardGetLSB(OUR(King)), s));
  }

  // King branch
  moves = LookupTableAttacks(l, BitBoardGetLSB(OUR(King)), King, EMPTY_BOARD) & ~US & ~attacked;
  b1 = (cb->castling | (attacked & ATTACK_MASK) | (ALL & OCCUPANCY_MASK)) & BACK_RANK(cb->turn);
  if ((b1 & KINGSIDE) == (KINGSIDE_CASTLING & BACK_RANK(cb->turn)) && (~checkMask == EMPTY_BOARD))
    moves |= OUR(King) << 2;
  if ((b1 & QUEENSIDE) == (QUEENSIDE_CASTLING & BACK_RANK(cb->turn)) && (~checkMask == EMPTY_BOARD))
    moves |= OUR(King) >> 2;
  bs->branches[bs->end++] = BranchNew(moves, OUR(King), GET_PIECE(King, cb->turn));

  // Piece branches
  b1 = US & ~(OUR(Pawn) | OUR(King));
  while (b1)
  {
    s = BitBoardPopLSB(&b1);
    moves = LookupTableAttacks(l, s, GET_TYPE(cb->squares[s]), ALL) & ~US & checkMask;
    if (BitBoardSetBit(EMPTY_BOARD, s) & pinned)
    {
      moves &= LookupTableGetLineOfSight(l, BitBoardGetLSB(OUR(King)), s);
    }
    bs->branches[bs->end++] = BranchNew(moves, BitBoardSetBit(EMPTY_BOARD, s), cb->squares[s]);
  }

  // Pawn branches
  b1 = OUR(Pawn);
  moves = PAWN_ATTACKS_LEFT(b1, cb->turn) & THEM & checkMask;
  bs->branches[bs->end++] = BranchNew(moves, PAWN_ATTACKS_LEFT(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));
  moves = PAWN_ATTACKS_RIGHT(b1, cb->turn) & THEM & checkMask;
  bs->branches[bs->end++] = BranchNew(moves, PAWN_ATTACKS_RIGHT(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));
  b2 = SINGLE_PUSH(b1, cb->turn) & ~ALL;
  moves = b2 & checkMask;
  bs->branches[bs->end++] = BranchNew(moves, SINGLE_PUSH(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));
  moves = SINGLE_PUSH(b2 & ENPASSANT_RANK(cb->turn), cb->turn) & ~ALL & checkMask;
  bs->branches[bs->end++] = BranchNew(moves, DOUBLE_PUSH(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));

  int baseIndex = bs->end - 4;
  b1 = OUR(Pawn) & pinned;
  while (b1)
  {
    s = BitBoardPopLSB(&b1);
    b2 = BitBoardSetBit(EMPTY_BOARD, s);
    b3 = LookupTableGetLineOfSight(l, BitBoardGetLSB(OUR(King)), s);

    // Remove any attacks that aren't on the pin mask from the set
    bs->branches[baseIndex + 0].to &= ~(PAWN_ATTACKS_LEFT(b2, cb->turn) & ~b3);
    bs->branches[baseIndex + 0].from = PAWN_ATTACKS_LEFT(bs->branches[baseIndex + 0].to, (!cb->turn));
    bs->branches[baseIndex + 1].to &= ~(PAWN_ATTACKS_RIGHT(b2, cb->turn) & ~b3);
    bs->branches[baseIndex + 1].from = PAWN_ATTACKS_RIGHT(bs->branches[baseIndex + 1].to, (!cb->turn));
    bs->branches[baseIndex + 2].to &= ~(SINGLE_PUSH(b2, cb->turn) & ~b3);
    bs->branches[baseIndex + 2].from = SINGLE_PUSH(bs->branches[baseIndex + 2].to, (!cb->turn));
    bs->branches[baseIndex + 3].to &= ~(DOUBLE_PUSH(b2, cb->turn) & ~b3);
    bs->branches[baseIndex + 3].from = DOUBLE_PUSH(bs->branches[baseIndex + 3].to, (!cb->turn));
  }

  // En passant branch
  if (cb->enPassant != EMPTY_SQUARE)
  {
    b1 = PAWN_ATTACKS(BitBoardSetBit(EMPTY_BOARD, cb->enPassant), (!cb->turn)) & OUR(Pawn);
    b2 = EMPTY_BOARD;
    b3 = BitBoardSetBit(EMPTY_BOARD, cb->enPassant);
    while (b1)
    {
      s = BitBoardPopLSB(&b1);
      // Check pseudo-pinning for en passant
      if ((LookupTableAttacks(l, BitBoardGetLSB(OUR(King)), Rook, ALL & ~BitBoardSetBit(SINGLE_PUSH(b3, (!cb->turn)), s)) & GET_RANK(BitBoardGetLSB(OUR(King))) & (THEIR(Rook) | THEIR(Queen))))
        continue;
      b2 |= BitBoardSetBit(EMPTY_BOARD, s);
      if (b2 & pinned)
        b2 &= LookupTableGetLineOfSight(l, BitBoardGetLSB(OUR(King)), cb->enPassant);
    }
    if (b2 != EMPTY_BOARD)
    {
      bs->branches[bs->end++] = BranchNew(b3, b2, GET_PIECE(Pawn, cb->turn));
    }
  }

  // Tighten the start/end bounds
  while ((bs->branches[bs->end].to == EMPTY_BOARD || bs->branches[bs->end].from == EMPTY_BOARD) && (bs->end > 0))
    bs->end--;

  while ((bs->branches[bs->start].to == EMPTY_BOARD || bs->branches[bs->start].from == EMPTY_BOARD) && (bs->start < (BRANCHES_SIZE - 1)))
    bs->start++;


  if (bs->end < bs->start) { // Moveset is empty so set bounds to sensible values
    bs->start = 0;
    bs->end = 0;
  }
}

int BranchCount(BranchSet *bs)
{
  int nodes = 0;
  for (int i = bs->start; i <= bs->end; i++)
  {
    if (bs->branches[i].to == EMPTY_BOARD || bs->branches[i].from == EMPTY_BOARD)
      continue;
    int x = BitBoardCountBits(bs->branches[i].to);
    int y = BitBoardCountBits(bs->branches[i].from);
    int offset = x - y;
    Type t = GET_TYPE(bs->branches[i].moved);
    Color c = GET_COLOR(bs->branches[i].moved);

    BitBoard promotion = EMPTY_BOARD;
    if (t == Pawn)
      promotion |= (PROMOTING_RANK(c) & bs->branches[i].to);
    if (offset < 0)
      nodes++;
    nodes += BitBoardCountBits(bs->branches[i].to) + BitBoardCountBits(promotion) * 3;
  }
  return nodes;
}


// Assumes moveset is not empty
Move BranchPop(BranchSet *bs)
{
  while (bs->branches[bs->start].to == EMPTY_BOARD || bs->branches[bs->start].from == EMPTY_BOARD)
    bs->start++;

  int i = bs->start;
  int x = BitBoardCountBits(bs->branches[i].to);
  int y = BitBoardCountBits(bs->branches[i].from);
  int offset = x - y; // Determines type of mapping

  Move m;
  m.from = BitBoardPopLSB(&bs->branches[i].from);
  m.to = BitBoardPopLSB(&bs->branches[i].to);
  m.moved = bs->branches[i].moved;
  if (offset > 0)
    bs->branches[i].from = BitBoardSetBit(bs->branches[i].from, m.from);
  else if (offset < 0)
    bs->branches[i].to = BitBoardSetBit(bs->branches[i].to, m.to);
  Color c = GET_COLOR(m.moved);
  Type t = GET_TYPE(m.moved);
  int promotion = (t == Pawn) && (BitBoardSetBit(EMPTY_BOARD, m.to) & PROMOTING_RANK(c));

  if (promotion) {
    // Last move wasn't the same move - Must be first promotion popped
    if ((bs->prev.from != m.from) || (bs->prev.to != m.to)) {
      m.moved = GET_PIECE(Knight, c);

      // Put the move back in the branch set
      bs->branches[i].from = BitBoardSetBit(bs->branches[i].from, m.from);
      bs->branches[i].to = BitBoardSetBit(bs->branches[i].to, m.to);
    } else {
      Type prevType = GET_TYPE(bs->prev.moved);
      m.moved = GET_PIECE((prevType + 1), c);
      // Last move that will be popped for this promotion
      if (prevType != Rook) {
        // Put the move back in the branch set
        bs->branches[i].from = BitBoardSetBit(bs->branches[i].from, m.from);
        bs->branches[i].to = BitBoardSetBit(bs->branches[i].to, m.to);
      }
    }
  }

  bs->prev = m;
  return m;
}

int BranchIsEmpty(BranchSet *bs) {
  return ((bs->start == bs->end) && (bs->branches[bs->start].to == EMPTY_BOARD || bs->branches[bs->start].from == EMPTY_BOARD));
}