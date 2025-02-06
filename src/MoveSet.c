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

static void addMap(MoveSet *ms, BitBoard to, BitBoard from, Piece moved);

// Add the map to moveset if it's non empty
static void addMap(MoveSet *ms, BitBoard to, BitBoard from, Piece moved)
{
  if (to == EMPTY_BOARD || from == EMPTY_BOARD)
    return;
  Map m;
  m.to = to;
  m.from = from;
  m.moved = moved;
  ms->maps[ms->size++] = m;
}

MoveSet MoveSetNew()
{
  MoveSet ms;
  ms.size = 0;
  ms.prev.moved = EMPTY_PIECE;
  ms.prev.from = EMPTY_SQUARE;
  ms.prev.to = EMPTY_SQUARE;
  return ms;
}

void MoveSetFill(LookupTable l, ChessBoard *cb, MoveSet *ms)
{
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
  addMap(ms, moves, OUR(King), GET_PIECE(King, cb->turn));

  // Piece maps
  b1 = US & ~(OUR(Pawn) | OUR(King));
  while (b1)
  {
    s = BitBoardPop(&b1);
    moves = LookupTableAttacks(l, s, GET_TYPE(cb->squares[s]), ALL) & ~US & checkMask;
    if (BitBoardAdd(EMPTY_BOARD, s) & pinned)
      moves &= LookupTableLineOfSight(l, BitBoardPeek(OUR(King)), s);
    addMap(ms, moves, BitBoardAdd(EMPTY_BOARD, s), cb->squares[s]);
  }

  // Pawn maps
  b1 = OUR(Pawn) & ~pinned;
  moves = PAWN_ATTACKS_LEFT(b1, cb->turn) & THEM & checkMask;
  addMap(ms, moves, PAWN_ATTACKS_LEFT(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));
  moves = PAWN_ATTACKS_RIGHT(b1, cb->turn) & THEM & checkMask;
  addMap(ms, moves, PAWN_ATTACKS_RIGHT(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));
  b2 = SINGLE_PUSH(b1, cb->turn) & ~ALL;
  moves = b2 & checkMask;
  addMap(ms, moves, SINGLE_PUSH(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));
  moves = SINGLE_PUSH(b2 & ENPASSANT_RANK(cb->turn), cb->turn) & ~ALL & checkMask;
  addMap(ms, moves, DOUBLE_PUSH(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));
  b1 = OUR(Pawn) & pinned;
  while (b1)
  {
    s = BitBoardPop(&b1);
    b3 = LookupTableLineOfSight(l, BitBoardPeek(OUR(King)), s);
    moves = PAWN_ATTACKS(BitBoardAdd(EMPTY_BOARD, s), cb->turn) & THEM;
    b2 = SINGLE_PUSH(BitBoardAdd(EMPTY_BOARD, s), cb->turn) & ~ALL;
    moves |= b2;
    moves |= SINGLE_PUSH(b2 & ENPASSANT_RANK(cb->turn), cb->turn) & ~ALL;
    addMap(ms, moves & b3 & checkMask, BitBoardAdd(EMPTY_BOARD, s), GET_PIECE(Pawn, cb->turn));
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
      addMap(ms, b3, b2, GET_PIECE(Pawn, cb->turn));
  }
}

int MoveSetCount(MoveSet *ms)
{
  int nodes = 0;
  for (int i = 0; i < ms->size; i++)
  {
    int offset = BitBoardCount(ms->maps[i].to) - BitBoardCount(ms->maps[i].from);
    BitBoard promotion = EMPTY_BOARD;
    if (GET_TYPE(ms->maps[i].moved) == Pawn)
      promotion |= (BACK_RANK(!GET_COLOR(ms->maps[i].moved)) & ms->maps[i].to);
    if (offset < 0)
      nodes++;
    nodes += BitBoardCount(ms->maps[i].to) + BitBoardCount(promotion) * 3;
  }
  return nodes;
}

// Assumes moveset is not empty
Move MoveSetPop(MoveSet *ms)
{
  int i = ms->size - 1;
  int offset = BitBoardCount(ms->maps[i].to) - BitBoardCount(ms->maps[i].from);
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

  // Check whether the map is now empty
  if (ms->maps[i].to == EMPTY_BOARD)
    ms->size--;

  ms->prev = m;
  return m;
}

int MoveSetIsEmpty(MoveSet *ms)
{
  return ms->size == 0;
}

int MoveSetMultiply(LookupTable l, ChessBoard *cb, MoveSet *ms)
{
  int moveDelta = 0;

  MoveSet curr; // Copy of ms
  memcpy(&curr, ms, sizeof(MoveSet));
  MoveSet next = MoveSetNew();
  ChessBoard temp = ChessBoardFlip(cb); // cb is unchanged
  MoveSetFill(l, &temp, &next);

  Square s1, s2;
  BitBoard b;
  BitBoard canBlock, isBlocking, canCapture;
  BitBoard isAttacking = EMPTY_BOARD, canAttack = EMPTY_BOARD;
  BitBoard theirMoves = EMPTY_BOARD;

  for (int i = 0; i < next.size; i++)
    theirMoves |= next.maps[i].to;

  for (int i = 0; i < curr.size; i++)
  {
    isBlocking = curr.maps[i].from & theirMoves; // Our piece(s) that blocks their piece move(s)
    canBlock = curr.maps[i].to & theirMoves;     // Our piece move(s) that could block their piece move(s)
    canCapture = curr.maps[i].to & THEM;         // Our piece moves(s) that could capture their piece(s)

    if (canBlock)
      curr.maps[i].to &= ~canBlock;
    if (isBlocking)
      curr.maps[i].to = EMPTY_BOARD;
    if (canCapture)
      curr.maps[i].to &= ~canCapture;
  }

  BitBoard kingRelevant = LookupTableAttacks(l, BitBoardPeek(THEIR(King)), King, THEM) |
                          BitBoardAdd(EMPTY_BOARD, BitBoardPeek(THEIR(King)));
  while (kingRelevant)
  {
    s1 = BitBoardPop(&kingRelevant);
    // Iterate through maps and remove moves that could attack the king square
    for (int i = 0; i < ms->size; i++)
    {
      Type t = GET_TYPE(ms->maps[i].moved);
      if (t == Pawn)
        continue;
      b = (t > Knight) ? LookupTableLineOfSight(l, s1, BitBoardPeek(ms->maps[i].from)) : EMPTY_BOARD;
      canAttack = ms->maps[i].to & LookupTableAttacks(l, s1, t, EMPTY_BOARD) & ~b;
      curr.maps[i].to &= ~canAttack;
    }

    // Iterate through pieces and find all pieces that are directly attacking the king square
    for (Type t = Pawn; t <= Queen; t++)
    {
      if (t == Pawn)
        continue;
      isAttacking |= OUR(t) & LookupTableAttacks(l, s1, t, EMPTY_BOARD);
    }
    while (isAttacking)
    {
      s2 = BitBoardPop(&isAttacking);
      b = LookupTableSquaresBetween(l, s1, s2);

      for (int i = 0; i < ms->size; i++)
      {
        if (ms->maps[i].from & b) // Our pieces can't move from the pin mask
          curr.maps[i].to &= b;
        curr.maps[i].to &= ~b; // Our pieces can't disrupt pin

        if (curr.maps[i].from == BitBoardAdd(EMPTY_BOARD, s2))
          curr.maps[i].to &= b; // Pinning piece can't move away from pin
      }
    }
  }

  // Remove curr from ms - wont work for pawns
  for (int i = 0; i < ms->size; i++)
    ms->maps[i].to &= ~curr.maps[i].to;

  // Remove any empty maps from ms
  int i = 0;
  while (i < ms->size)
  {
    if ((ms->maps[i].to == EMPTY_BOARD) || (ms->maps[i].from == EMPTY_BOARD))
    {
      ms->maps[i] = ms->maps[ms->size - 1];
      ms->size--;
    }
    else
    {
      i++;
    }
  }

  // Remove any empty maps from curr
  i = 0;
  while (i < curr.size)
  {
    if ((curr.maps[i].to == EMPTY_BOARD) || (curr.maps[i].from == EMPTY_BOARD))
    {
      curr.maps[i] = curr.maps[curr.size - 1]; // Move last element to current index
      curr.size--;                             // Reduce the size
    }
    else
    {
      i++; // Only increment if no removal happened
    }
  }

  // Print curr
  // printf("Curr\n");
  // for (int i = 0; i < curr.size; i++)
  // {
  //   printf("Map %d\n", i);
  //   BitBoardPrint(curr.maps[i].to);
  //   BitBoardPrint(curr.maps[i].from);
  //   printf("Moved: %d\n", curr.maps[i].moved);
  // }
  // printf("We claim that %d moves would've been played\n", MoveSetCount(&curr) * MoveSetCount(&next) + moveDelta);

  return MoveSetCount(&curr) * MoveSetCount(&next) + moveDelta;
}