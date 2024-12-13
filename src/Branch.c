#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include "Branch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Branch BranchNew(BitBoard to, BitBoard from, Piece moved)
{
  Branch b;
  b.to = to;
  b.from = from;
  b.moved = moved;
  return b;
}

int BranchFill(LookupTable l, ChessBoard *cb, Branch *b)
{
  int size = 0;
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
  b[size++] = BranchNew(moves, OUR(King), GET_PIECE(King, cb->turn));

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
    b[size++] = BranchNew(moves, BitBoardSetBit(EMPTY_BOARD, s), cb->squares[s]);
  }

  // Pawn branches
  b1 = OUR(Pawn);
  moves = PAWN_ATTACKS_LEFT(b1, cb->turn) & THEM & checkMask;
  b[size++] = BranchNew(moves, PAWN_ATTACKS_LEFT(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));
  moves = PAWN_ATTACKS_RIGHT(b1, cb->turn) & THEM & checkMask;
  b[size++] = BranchNew(moves, PAWN_ATTACKS_RIGHT(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));
  b2 = SINGLE_PUSH(b1, cb->turn) & ~ALL;
  moves = b2 & checkMask;
  b[size++] = BranchNew(moves, SINGLE_PUSH(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));
  moves = SINGLE_PUSH(b2 & ENPASSANT_RANK(cb->turn), cb->turn) & ~ALL & checkMask;
  b[size++] = BranchNew(moves, DOUBLE_PUSH(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));

  {
    int baseIndex = size - 4;
    b1 = OUR(Pawn) & pinned;
    while (b1)
    {
      s = BitBoardPopLSB(&b1);
      b2 = BitBoardSetBit(EMPTY_BOARD, s);
      b3 = LookupTableGetLineOfSight(l, BitBoardGetLSB(OUR(King)), s);

      // Remove any attacks that aren't on the pin mask from the set
      b[baseIndex + 0].to &= ~(PAWN_ATTACKS_LEFT(b2, cb->turn) & ~b3);
      b[baseIndex + 0].from = PAWN_ATTACKS_LEFT(b[baseIndex + 0].to, (!cb->turn));
      b[baseIndex + 1].to &= ~(PAWN_ATTACKS_RIGHT(b2, cb->turn) & ~b3);
      b[baseIndex + 1].from = PAWN_ATTACKS_RIGHT(b[baseIndex + 1].to, (!cb->turn));
      b[baseIndex + 2].to &= ~(SINGLE_PUSH(b2, cb->turn) & ~b3);
      b[baseIndex + 2].from = SINGLE_PUSH(b[baseIndex + 2].to, (!cb->turn));
      b[baseIndex + 3].to &= ~(DOUBLE_PUSH(b2, cb->turn) & ~b3);
      b[baseIndex + 3].from = DOUBLE_PUSH(b[baseIndex + 3].to, (!cb->turn));
    }
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
      b[size++] = BranchNew(b3, b2, GET_PIECE(Pawn, cb->turn));
    }
  }

  return size;
}

int BranchCount(Branch *b, int size)
{
  int nodes = 0;
  for (int i = 0; i < size; i++)
  {
    if ((b[i].to == EMPTY_BOARD || b[i].from == EMPTY_BOARD))
      continue;
    int x = BitBoardCountBits(b[i].to);
    int y = BitBoardCountBits(b[i].from);
    int offset = x - y;
    Type t = GET_TYPE(b[i].moved);
    Color c = GET_COLOR(b[i].moved);

    BitBoard promotion = EMPTY_BOARD;
    if (t == Pawn)
      promotion |= (PROMOTING_RANK(c) & b[i].to);
    if (offset < 0)
      nodes++;
    nodes += BitBoardCountBits(b[i].to) + BitBoardCountBits(promotion) * 3;
  }
  return nodes;
}

int BranchExtract(Branch *b, int size, Move *moves)
{
  Move m;
  int index = 0;
  for (int i = 0; i < size; i++)
  {
    if ((b[i].to == EMPTY_BOARD || b[i].from == EMPTY_BOARD))
      continue;
    int x = BitBoardCountBits(b[i].to);
    int y = BitBoardCountBits(b[i].from);
    int offset = x - y;
    int max = (x > y) ? x : y;

    BitBoard toCopy = b[i].to;
    BitBoard fromCopy = b[i].from;

    for (int j = 0; j < max; j++)
    {
      m.from = BitBoardPopLSB(&fromCopy);
      m.to = BitBoardPopLSB(&toCopy);
      m.moved = b[i].moved;
      if (offset > 0)
        fromCopy = BitBoardSetBit(fromCopy, m.from);
      else if (offset < 0)
        toCopy = BitBoardSetBit(toCopy, m.to);
      Color c = GET_COLOR(m.moved);
      Type t = GET_TYPE(m.moved);
      int promotion = (t == Pawn) && (BitBoardSetBit(EMPTY_BOARD, m.to) & PROMOTING_RANK(c));
      if (promotion)
        m.moved = GET_PIECE(Knight, c);

      if (promotion)
      {
        for (Type tt = Knight; tt <= Queen; tt++)
        {
          m.moved = GET_PIECE(tt, c);
          moves[index++] = m;
        }
      }
      else
      {
        moves[index++] = m;
      }
    }
  }
  return index;
}

/*
 * Suppose we're two moves before a leaf node, and we have an array of branches for the current
 * moves that can be played. If any of the moves in a current branch don't interfere with all the
 * moves in the next branch, we can multiply the total number of moves that don't interfere with
 * the number of moves in the next branch. This means we wouldn't have to explore the next branch
 * for these moves at all.
 */
int BranchPrune(LookupTable l, ChessBoard *cb, Branch *curr, int currSize)
{

  // Your code here

  (void)l;
  (void)cb;
  (void)curr;
  (void)currSize;
  return 0;
}