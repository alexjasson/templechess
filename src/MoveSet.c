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
#define ENPASSANT_RANK(c) (BitBoard) SOUTH_EDGE >> (EDGE_SIZE * ((c * 3) + 2))
#define PROMOTION_RANK(c) (BitBoard) SOUTH_EDGE >> (EDGE_SIZE * ((!c * 5) + 1))
#define BACK_RANK(c) (BitBoard)((c == White) ? SOUTH_EDGE : NORTH_EDGE)

// Returns a bitboard representing a set of moves given a set of pawns and a color
#define PAWN_ATTACKS(b, c) ((c == White) ? BitBoardShiftNW(b) | BitBoardShiftNE(b) : BitBoardShiftSW(b) | BitBoardShiftSE(b))
#define PAWN_ATTACKS_LEFT(b, c) ((c == White) ? BitBoardShiftNW(b) : BitBoardShiftSE(b))
#define PAWN_ATTACKS_RIGHT(b, c) ((c == White) ? BitBoardShiftNE(b) : BitBoardShiftSW(b))
#define SINGLE_PUSH(b, c) ((c == White) ? BitBoardShiftN(b) : BitBoardShiftS(b))
#define DOUBLE_PUSH(b, c) ((c == White) ? BitBoardShiftN(BitBoardShiftN(b)) : BitBoardShiftS(BitBoardShiftS(b)))

static void addMap(MoveSet *ms, BitBoard to, BitBoard from, Piece moved);
static BitBoard pawnMoves(BitBoard p, Color c);

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

static BitBoard pawnMoves(BitBoard p, Color c)
{
  BitBoard moves = SINGLE_PUSH(p, c);
  moves |= SINGLE_PUSH(moves & ENPASSANT_RANK(c), c);
  moves |= PAWN_ATTACKS(p, c);
  return moves;
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
  if ((~checkMask) == EMPTY_BOARD) {
    // Kingside: check rights and interior squares f/g
    int clear = (((attacked & ATTACK_MASK) | (ALL & OCCUPANCY_MASK)) & (KINGSIDE & ~KINGSIDE_CASTLING) & BACK_RANK(ChessBoardColor(cb))) == EMPTY_BOARD;
    if (ChessBoardKingSide(cb) && clear)
      moves |= OUR(King) << 2;
    // Queenside: check rights and interior squares b/c/d
    clear = (((attacked & ATTACK_MASK) | (ALL & OCCUPANCY_MASK)) & (QUEENSIDE & ~QUEENSIDE_CASTLING) & BACK_RANK(ChessBoardColor(cb))) == EMPTY_BOARD;
    if (ChessBoardQueenSide(cb) && clear)
      moves |= OUR(King) >> 2;
  }
  addMap(ms, moves, OUR(King), GET_PIECE(King, ChessBoardColor(cb)));

  // Piece maps
  b1 = US & ~(OUR(Pawn) | OUR(King));
  while (b1)
  {
    s = BitBoardPop(&b1);
    moves = LookupTableAttacks(l, s, GET_TYPE(ChessBoardSquare(cb, s)), ALL) & ~US & checkMask;
    if (BitBoardAdd(EMPTY_BOARD, s) & pinned)
      moves &= LookupTableLineOfSight(l, BitBoardPeek(OUR(King)), s);
    addMap(ms, moves, BitBoardAdd(EMPTY_BOARD, s), ChessBoardSquare(cb, s));
  }

  // Pawn maps
  b1 = OUR(Pawn) & ~pinned;
  moves = PAWN_ATTACKS_LEFT(b1, ChessBoardColor(cb)) & THEM & checkMask;
  addMap(ms, moves, PAWN_ATTACKS_LEFT(moves, (!ChessBoardColor(cb))), GET_PIECE(Pawn, ChessBoardColor(cb)));
  moves = PAWN_ATTACKS_RIGHT(b1, ChessBoardColor(cb)) & THEM & checkMask;
  addMap(ms, moves, PAWN_ATTACKS_RIGHT(moves, (!ChessBoardColor(cb))), GET_PIECE(Pawn, ChessBoardColor(cb)));
  b2 = SINGLE_PUSH(b1, ChessBoardColor(cb)) & ~ALL;
  moves = b2 & checkMask;
  addMap(ms, moves, SINGLE_PUSH(moves, (!ChessBoardColor(cb))), GET_PIECE(Pawn, ChessBoardColor(cb)));
  moves = SINGLE_PUSH(b2 & ENPASSANT_RANK(ChessBoardColor(cb)), ChessBoardColor(cb)) & ~ALL & checkMask;
  addMap(ms, moves, DOUBLE_PUSH(moves, (!ChessBoardColor(cb))), GET_PIECE(Pawn, ChessBoardColor(cb)));
  b1 = OUR(Pawn) & pinned;
  while (b1)
  {
    s = BitBoardPop(&b1);
    b3 = LookupTableLineOfSight(l, BitBoardPeek(OUR(King)), s);
    moves = PAWN_ATTACKS(BitBoardAdd(EMPTY_BOARD, s), ChessBoardColor(cb)) & THEM;
    b2 = SINGLE_PUSH(BitBoardAdd(EMPTY_BOARD, s), ChessBoardColor(cb)) & ~ALL;
    moves |= b2;
    moves |= SINGLE_PUSH(b2 & ENPASSANT_RANK(ChessBoardColor(cb)), ChessBoardColor(cb)) & ~ALL;
    addMap(ms, moves & b3 & checkMask, BitBoardAdd(EMPTY_BOARD, s), GET_PIECE(Pawn, ChessBoardColor(cb)));
  }

  // En passant map
  if (ChessBoardEnPassant(cb) != EMPTY_SQUARE)
  {
    b1 = PAWN_ATTACKS(BitBoardAdd(EMPTY_BOARD, ChessBoardEnPassant(cb)), (!ChessBoardColor(cb))) & OUR(Pawn);
    b2 = EMPTY_BOARD;
    b3 = BitBoardAdd(EMPTY_BOARD, ChessBoardEnPassant(cb));
    while (b1)
    {
      s = BitBoardPop(&b1);
      // Check pseudo-pinning for en passant
      if (LookupTableAttacks(l, BitBoardPeek(OUR(King)), Rook, ALL & ~BitBoardAdd(SINGLE_PUSH(b3, (!ChessBoardColor(cb))), s)) &
          RANK_OF(BitBoardPeek(OUR(King))) & (THEIR(Rook) | THEIR(Queen)))
        continue;
      b2 |= BitBoardAdd(EMPTY_BOARD, s);
      if (b2 & pinned)
        b2 &= LookupTableLineOfSight(l, BitBoardPeek(OUR(King)), ChessBoardEnPassant(cb));
    }
    if (b2 != EMPTY_BOARD)
      addMap(ms, b3, b2, GET_PIECE(Pawn, ChessBoardColor(cb)));
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

void MoveSetPrint(MoveSet ms)
{
  printf("{");
  int first = 1;
  while (!MoveSetIsEmpty(&ms))
  {
    if (!first)
      printf(", ");
    first = 0;
    Move m = MoveSetPop(&ms);
    ChessBoardPrintMove(m);
  }
  printf("}\n");
}

int MoveSetMultiply(LookupTable l, ChessBoard *cb, MoveSet *ms)
{
  int prevCount = MoveSetCount(ms);
  ChessBoard temp = ChessBoardFlip(cb); // cb is unchanged
  MoveSet next = MoveSetNew();
  MoveSetFill(l, &temp, &next);

  BitBoard theirMoves = EMPTY_BOARD;  // To squares of their moves
  BitBoard special = EMPTY_BOARD;     // From squares of special pawn moves - en passant, promotion
  BitBoard castling = EMPTY_BOARD;    // To squares of castling moves
  BitBoard pinned = EMPTY_BOARD;      // Squares that are pinned
  BitBoard isAttacking = EMPTY_BOARD; // From squares that are attacking relevant king squares
  BitBoard canAttack[TYPE_SIZE];      // To squares that would attack relevant king squares for each piece type
  memset(canAttack, EMPTY_BOARD, sizeof(canAttack));

  // Consider their moves that could be disrupted by our moves
  theirMoves |= pawnMoves(THEIR(Pawn), !ChessBoardColor(cb));
  for (int i = 0; i < next.size; i++)
  {
    if (GET_TYPE(next.maps[i].moved) > Knight)
      theirMoves |= next.maps[i].to;
  }

  // Consider our moves that could disrupt their king moves
  BitBoard kingRelevant = (LookupTableAttacks(l, BitBoardPeek(THEIR(King)), King, EMPTY_BOARD) & ~THEM) |
                          BitBoardAdd(EMPTY_BOARD, BitBoardPeek(THEIR(King)));
  while (kingRelevant)
  {
    Square s1 = BitBoardPop(&kingRelevant);
    BitBoard ourPieces = US & ~OUR(Pawn);
    while (ourPieces)
    {
      Square s2 = BitBoardPop(&ourPieces);
      Type t = GET_TYPE(ChessBoardSquare(cb, s2));
      BitBoard piece = BitBoardAdd(EMPTY_BOARD, s2);
      BitBoard projection = LookupTableAttacks(l, s1, t, EMPTY_BOARD);
      BitBoard moves = LookupTableAttacks(l, s2, t, EMPTY_BOARD);
      canAttack[t] |= moves & projection;
      isAttacking |= piece & projection;
      if (piece & projection)
        pinned |= LookupTableSquaresBetween(l, s1, s2);
    }
    BitBoard king = BitBoardAdd(EMPTY_BOARD, s1);
    BitBoard projection = PAWN_ATTACKS(king, !ChessBoardColor(cb));
    BitBoard moves = pawnMoves(OUR(Pawn), ChessBoardColor(cb));
    canAttack[Pawn] |= moves & projection;
    isAttacking |= OUR(Pawn) & projection;
  }

  // Consider special moves that are annoying to calculate
  for (int i = 0; i < ms->size; i++)
  {
    Type t = GET_TYPE(ms->maps[i].moved);
    int squareOffset = BitBoardPeek(ms->maps[i].to) - BitBoardPeek(ms->maps[i].from);

    if (t == Pawn)
    {
      if (squareOffset == 16 || squareOffset == -16) // Double pushes causing en passant
        special |= SINGLE_PUSH(SINGLE_PUSH(ms->maps[i].from, ChessBoardColor(cb)) & PAWN_ATTACKS(THEIR(Pawn), !ChessBoardColor(cb)), !ChessBoardColor(cb));
      if (ChessBoardEnPassant(cb) != EMPTY_SQUARE) // En passant
        special |= PAWN_ATTACKS(BitBoardAdd(EMPTY_BOARD, ChessBoardEnPassant(cb)), !ChessBoardColor(cb)) & ms->maps[i].from;
      special |= ms->maps[i].from & PROMOTION_RANK(ChessBoardColor(cb)); // Promotion
    }
    else if (t == King) // Castling
      castling = ms->maps[i].to & ~LookupTableAttacks(l, BitBoardPeek(OUR(King)), King, EMPTY_BOARD);
  }

  // Remove moves from ms that can be easily multiplied by the next moveset
  for (int i = 0; i < ms->size; i++)
  {
    Type t = GET_TYPE(ms->maps[i].moved);
    int mapOffset = BitBoardCount(ms->maps[i].to) - BitBoardCount(ms->maps[i].from);

    if (mapOffset > 0) // Injective
    {
      if (!(ms->maps[i].from & (pinned | isAttacking | theirMoves)))
        ms->maps[i].to &= pinned | canAttack[t] | theirMoves | THEM | castling;
    }
    else if (mapOffset == 0) // Bijective
    {
      int squareOffset = BitBoardPeek(ms->maps[i].to) - BitBoardPeek(ms->maps[i].from);
      if (squareOffset > 0)
      {
        ms->maps[i].to &= pinned | canAttack[t] | theirMoves | THEM | ((pinned | isAttacking | theirMoves | special) << squareOffset);
        ms->maps[i].from = ms->maps[i].to >> squareOffset;
      }
      else // squareOffset < 0
      {
        ms->maps[i].to &= pinned | canAttack[t] | theirMoves | THEM | ((pinned | isAttacking | theirMoves | special) >> -squareOffset);
        ms->maps[i].from = ms->maps[i].to << -squareOffset;
      }
    }
  }

  // Remove empty maps from ms
  for (int i = 0; i < ms->size;)
  {
    if ((ms->maps[i].to == EMPTY_BOARD) || (ms->maps[i].from == EMPTY_BOARD))
      ms->maps[i] = ms->maps[--ms->size];
    else
      i++;
  }

  return (prevCount - MoveSetCount(ms)) * MoveSetCount(&next);
}