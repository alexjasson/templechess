#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include "MoveSet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// BitBoard representing the rank of a specific square
#define RANK_OF(s) (SOUTH_EDGE >> (EDGE_SIZE * (EDGE_SIZE - BitBoardRank(s) - 1)))

// Bitboards representing the ranks from the perspective of the given color c
#define ENPASSANT_RANK(c) (BitBoard) SOUTH_EDGE >> (EDGE_SIZE * ((c * 3) + 2)) // En
#define BACK_RANK(c) (BitBoard)((c == White) ? SOUTH_EDGE : NORTH_EDGE)

// Returns a bitboard representing a set of moves given a set of pawns and a color
#define PAWN_ATTACKS(b, c) ((c == White) ? BitBoardShiftNW(b) | BitBoardShiftNE(b) : BitBoardShiftSW(b) | BitBoardShiftSE(b))
#define PAWN_ATTACKS_LEFT(b, c) ((c == White) ? BitBoardShiftNW(b) : BitBoardShiftSE(b))
#define PAWN_ATTACKS_RIGHT(b, c) ((c == White) ? BitBoardShiftNE(b) : BitBoardShiftSW(b))
#define SINGLE_PUSH(b, c) ((c == White) ? BitBoardShiftN(b) : BitBoardShiftS(b))
#define DOUBLE_PUSH(b, c) ((c == White) ? BitBoardShiftN(BitBoardShiftN(b)) : BitBoardShiftS(BitBoardShiftS(b)))

static Map newMap(BitBoard to, BitBoard from, Piece moved);
static int isMapEmpty(Map m);

static Map newMap(BitBoard to, BitBoard from, Piece moved)
{
  Map b;
  b.to = to;
  b.from = from;
  b.moved = moved;
  return b;
}

static int isMapEmpty(Map m)
{
  return (m.to == EMPTY_BOARD) || (m.from == EMPTY_BOARD);
}

MoveSet MoveSetNew()
{
  MoveSet ms;
  ms.start = 0;
  ms.end = 0;
  ms.maps[0].to = EMPTY_BOARD;
  ms.maps[0].from = EMPTY_BOARD;
  ms.maps[0].moved = EMPTY_PIECE;
  return ms;
}

/*
 * Fill each map with the legal moves for each piece on the board. The first map stores an
 * injective mapping of all the legal king moves. The following maps store an injective
 * mapping of each of the piece moves. There are then 4 maps which stores a bijective mapping
 * of the left pawn attacks, right pawn attacks, single pawn pushes and double pawn pushes.
 * Finally, the last map stores a surjective mapping of en passant moves.
 */
void MoveSetFill(LookupTable l, ChessBoard *cb, MoveSet *ms)
{
  memset(ms, 0, sizeof(MoveSet));
  ms->prev.moved = EMPTY_PIECE;
  ms->prev.from = EMPTY_SQUARE;
  ms->prev.to = EMPTY_SQUARE;

  Square s;
  BitBoard pinned, checking, attacked, checkMask, moves, b1, b2, b3;

  attacked = ChessBoardAttacked(l, cb);
  checking = ChessBoardChecking(l, cb);
  pinned = ChessBoardPinned(l, cb);
  checkMask = ~EMPTY_BOARD;
  while (checking)
  {
    s = BitBoardPop(&checking);
    checkMask &= (BitBoardAdd(EMPTY_BOARD, s) | LookupTableSquaresBetween(l, BitBoardPeek(OUR(King)), s));
  }

  // King map
  moves = LookupTableAttacks(l, BitBoardPeek(OUR(King)), King, EMPTY_BOARD) & ~US & ~attacked;
  b1 = (cb->castling | (attacked & ATTACK_MASK) | (ALL & OCCUPANCY_MASK)) & BACK_RANK(cb->turn);
  if ((b1 & KINGSIDE) == (KINGSIDE_CASTLING & BACK_RANK(cb->turn)) && (~checkMask == EMPTY_BOARD))
    moves |= OUR(King) << 2;
  if ((b1 & QUEENSIDE) == (QUEENSIDE_CASTLING & BACK_RANK(cb->turn)) && (~checkMask == EMPTY_BOARD))
    moves |= OUR(King) >> 2;
  ms->maps[ms->end++] = newMap(moves, OUR(King), GET_PIECE(King, cb->turn));

  // Piece maps
  b1 = US & ~(OUR(Pawn) | OUR(King));
  while (b1)
  {
    s = BitBoardPop(&b1);
    moves = LookupTableAttacks(l, s, GET_TYPE(cb->squares[s]), ALL) & ~US & checkMask;
    if (BitBoardAdd(EMPTY_BOARD, s) & pinned)
      moves &= LookupTableLineOfSight(l, BitBoardPeek(OUR(King)), s);
    ms->maps[ms->end++] = newMap(moves, BitBoardAdd(EMPTY_BOARD, s), cb->squares[s]);
  }

  // Pawn maps
  b1 = OUR(Pawn);
  moves = PAWN_ATTACKS_LEFT(b1, cb->turn) & THEM & checkMask;
  ms->maps[ms->end++] = newMap(moves, PAWN_ATTACKS_LEFT(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));
  moves = PAWN_ATTACKS_RIGHT(b1, cb->turn) & THEM & checkMask;
  ms->maps[ms->end++] = newMap(moves, PAWN_ATTACKS_RIGHT(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));
  b2 = SINGLE_PUSH(b1, cb->turn) & ~ALL;
  moves = b2 & checkMask;
  ms->maps[ms->end++] = newMap(moves, SINGLE_PUSH(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));
  moves = SINGLE_PUSH(b2 & ENPASSANT_RANK(cb->turn), cb->turn) & ~ALL & checkMask;
  ms->maps[ms->end++] = newMap(moves, DOUBLE_PUSH(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));

  int baseIndex = ms->end - 4;
  b1 = OUR(Pawn) & pinned;
  while (b1)
  {
    s = BitBoardPop(&b1);
    b2 = BitBoardAdd(EMPTY_BOARD, s);
    b3 = LookupTableLineOfSight(l, BitBoardPeek(OUR(King)), s);

    // Remove any attacks that aren't on the pin mask from the set of squares
    ms->maps[baseIndex + 0].to &= ~(PAWN_ATTACKS_LEFT(b2, cb->turn) & ~b3);
    ms->maps[baseIndex + 0].from = PAWN_ATTACKS_LEFT(ms->maps[baseIndex + 0].to, (!cb->turn));
    ms->maps[baseIndex + 1].to &= ~(PAWN_ATTACKS_RIGHT(b2, cb->turn) & ~b3);
    ms->maps[baseIndex + 1].from = PAWN_ATTACKS_RIGHT(ms->maps[baseIndex + 1].to, (!cb->turn));
    ms->maps[baseIndex + 2].to &= ~(SINGLE_PUSH(b2, cb->turn) & ~b3);
    ms->maps[baseIndex + 2].from = SINGLE_PUSH(ms->maps[baseIndex + 2].to, (!cb->turn));
    ms->maps[baseIndex + 3].to &= ~(DOUBLE_PUSH(b2, cb->turn) & ~b3);
    ms->maps[baseIndex + 3].from = DOUBLE_PUSH(ms->maps[baseIndex + 3].to, (!cb->turn));
  }

  // En passant map
  if (cb->enPassant != EMPTY_SQUARE)
  {
    b1 = PAWN_ATTACKS(BitBoardAdd(EMPTY_BOARD, cb->enPassant), (!cb->turn)) & OUR(Pawn);
    b2 = EMPTY_BOARD;
    b3 = BitBoardAdd(EMPTY_BOARD, cb->enPassant);
    while (b1)
    {
      s = BitBoardPop(&b1);
      // Check pseudo-pinning for en passant
      if (LookupTableAttacks(l, BitBoardPeek(OUR(King)), Rook, ALL & ~BitBoardAdd(SINGLE_PUSH(b3, (!cb->turn)), s)) &
          RANK_OF(BitBoardPeek(OUR(King))) & (THEIR(Rook) | THEIR(Queen)))
        continue;
      b2 |= BitBoardAdd(EMPTY_BOARD, s);
      if (b2 & pinned)
        b2 &= LookupTableLineOfSight(l, BitBoardPeek(OUR(King)), cb->enPassant);
    }
    if (b2 != EMPTY_BOARD)
      ms->maps[ms->end++] = newMap(b3, b2, GET_PIECE(Pawn, cb->turn));
  }

  // Tighten the start/end bounds
  while (isMapEmpty(ms->maps[ms->end]) && (ms->end > 0))
    ms->end--;
  while (isMapEmpty(ms->maps[ms->end]) && (ms->start < (MAPS_SIZE - 1)))
    ms->start++;

  if (ms->end < ms->start) // Moveset is empty so set bounds to sensible values
  {
    ms->start = 0;
    ms->end = 0;
  }
}

int MoveSetCount(MoveSet *ms)
{
  int nodes = 0;
  for (int i = ms->start; i <= ms->end; i++)
  {
    if (ms->maps[i].to == EMPTY_BOARD || ms->maps[i].from == EMPTY_BOARD)
      continue;
    int x = BitBoardCount(ms->maps[i].to);
    int y = BitBoardCount(ms->maps[i].from);
    int offset = x - y;
    Type t = GET_TYPE(ms->maps[i].moved);
    Color c = GET_COLOR(ms->maps[i].moved);

    BitBoard promotion = EMPTY_BOARD;
    if (t == Pawn)
      promotion |= (BACK_RANK(!c) & ms->maps[i].to);
    if (offset < 0)
      nodes++;
    nodes += BitBoardCount(ms->maps[i].to) + BitBoardCount(promotion) * 3;
  }
  return nodes;
}

// Assumes moveset is not empty
Move MoveSetPop(MoveSet *ms)
{
  while (isMapEmpty(ms->maps[ms->start]))
    ms->start++;

  int i = ms->start;
  int x = BitBoardCount(ms->maps[i].to);
  int y = BitBoardCount(ms->maps[i].from);
  int offset = x - y; // Determines type of mapping

  Move m;
  m.from = BitBoardPop(&ms->maps[i].from);
  m.to = BitBoardPop(&ms->maps[i].to);
  m.moved = ms->maps[i].moved;
  if (offset > 0)
    ms->maps[i].from = BitBoardAdd(ms->maps[i].from, m.from);
  else if (offset < 0)
    ms->maps[i].to = BitBoardAdd(ms->maps[i].to, m.to);
  Color c = GET_COLOR(m.moved);
  Type t = GET_TYPE(m.moved);
  int promotion = (t == Pawn) && (BitBoardAdd(EMPTY_BOARD, m.to) & BACK_RANK(!c));

  if (promotion)
  {
    // Last move wasn't the same move - Must be first promotion popped
    if ((ms->prev.from != m.from) || (ms->prev.to != m.to))
    {
      m.moved = GET_PIECE(Knight, c);

      // Put the move back in the move set
      ms->maps[i].from = BitBoardAdd(ms->maps[i].from, m.from);
      ms->maps[i].to = BitBoardAdd(ms->maps[i].to, m.to);
    }
    else
    {
      Type prevType = GET_TYPE(ms->prev.moved);
      m.moved = GET_PIECE((prevType + 1), c);
      // Last move that will be popped for this promotion
      if (prevType != Rook)
      {
        // Put the move back in the move set
        ms->maps[i].from = BitBoardAdd(ms->maps[i].from, m.from);
        ms->maps[i].to = BitBoardAdd(ms->maps[i].to, m.to);
      }
    }
  }

  ms->prev = m;
  return m;
}

int MoveSetIsEmpty(MoveSet *ms)
{
  return ((ms->start == ms->end) && (ms->maps[ms->start].to == EMPTY_BOARD || ms->maps[ms->start].from == EMPTY_BOARD));
}