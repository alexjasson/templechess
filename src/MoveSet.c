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

static void addMap(MoveSet *ms, BitBoard to, BitBoard from, Type type);
static BitBoard pawnMoves(BitBoard p, Color c);

// Add the map to moveset if it's non empty
static void addMap(MoveSet *ms, BitBoard to, BitBoard from, Type type)
{
  if (to == EMPTY_BOARD || from == EMPTY_BOARD)
    return;
  Map m;
  m.to = to;
  m.from = from;
  m.type = type;
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
  // Initialize prev move as empty
  ms.prev.from.square = EMPTY_SQUARE;
  ms.prev.from.type = Empty;
  ms.prev.to.square = EMPTY_SQUARE;
  ms.prev.to.type = Empty;
  ms.prev.captured.square = EMPTY_SQUARE;
  ms.prev.captured.type = Empty;
  ms.prev.enPassant = EMPTY_SQUARE;
  ms.prev.castling = 0;
  ms.cb = NULL;
  return ms;
}

void MoveSetFill(LookupTable l, ChessBoard *cb, MoveSet *ms)
{
  // Store board pointer for MoveSetPop
  ms->cb = cb;
  Square s;
  BitBoard pinned, checking, attacked, checkMask, moves, b1, b2, b3;

  attacked = ChessBoardAttacked(l, cb);
  checking = ChessBoardChecking(l, cb);
  pinned = ChessBoardPinned(l, cb);
  checkMask = ~EMPTY_BOARD;
  while (checking)
  {
    s = BitBoardPop(&checking);
    checkMask &= (BitBoardAdd(EMPTY_BOARD, s) | LookupTableSquaresBetween(l, BitBoardPeek(ChessBoardOur(cb, King)), s));
  }

  // King map
  moves = LookupTableAttacks(l, BitBoardPeek(ChessBoardOur(cb, King)), King, EMPTY_BOARD) & ~ChessBoardUs(cb) & ~attacked;
  if ((~checkMask) == EMPTY_BOARD) {
    // Kingside: check rights and interior squares f/g
    int clear = (((attacked & ATTACK_MASK) | (ChessBoardAll(cb) & OCCUPANCY_MASK)) & (KINGSIDE & ~KINGSIDE_CASTLING) & BACK_RANK(ChessBoardColor(cb))) == EMPTY_BOARD;
    if (ChessBoardKingSide(cb) && clear)
      moves |= ChessBoardOur(cb, King) << 2;
    // Queenside: check rights and interior squares b/c/d
    clear = (((attacked & ATTACK_MASK) | (ChessBoardAll(cb) & OCCUPANCY_MASK)) & (QUEENSIDE & ~QUEENSIDE_CASTLING) & BACK_RANK(ChessBoardColor(cb))) == EMPTY_BOARD;
    if (ChessBoardQueenSide(cb) && clear)
      moves |= ChessBoardOur(cb, King) >> 2;
  }
  addMap(ms, moves, ChessBoardOur(cb, King), King);

  // Piece maps
  b1 = ChessBoardUs(cb) & ~(ChessBoardOur(cb, Pawn) | ChessBoardOur(cb, King));
  while (b1)
  {
    s = BitBoardPop(&b1);
    moves = LookupTableAttacks(l, s, ChessBoardSquare(cb, s), ChessBoardAll(cb)) & ~ChessBoardUs(cb) & checkMask;
    if (BitBoardAdd(EMPTY_BOARD, s) & pinned)
      moves &= LookupTableLineOfSight(l, BitBoardPeek(ChessBoardOur(cb, King)), s);
    addMap(ms, moves, BitBoardAdd(EMPTY_BOARD, s), ChessBoardSquare(cb, s));
  }

  // Pawn maps
  b1 = ChessBoardOur(cb, Pawn) & ~pinned;
  moves = PAWN_ATTACKS_LEFT(b1, ChessBoardColor(cb)) & ChessBoardThem(cb) & checkMask;
  addMap(ms, moves, PAWN_ATTACKS_LEFT(moves, (!ChessBoardColor(cb))), Pawn);
  moves = PAWN_ATTACKS_RIGHT(b1, ChessBoardColor(cb)) & ChessBoardThem(cb) & checkMask;
  addMap(ms, moves, PAWN_ATTACKS_RIGHT(moves, (!ChessBoardColor(cb))), Pawn);
  b2 = SINGLE_PUSH(b1, ChessBoardColor(cb)) & ~ChessBoardAll(cb);
  moves = b2 & checkMask;
  addMap(ms, moves, SINGLE_PUSH(moves, (!ChessBoardColor(cb))), Pawn);
  moves = SINGLE_PUSH(b2 & ENPASSANT_RANK(ChessBoardColor(cb)), ChessBoardColor(cb)) & ~ChessBoardAll(cb) & checkMask;
  addMap(ms, moves, DOUBLE_PUSH(moves, (!ChessBoardColor(cb))), Pawn);
  b1 = ChessBoardOur(cb, Pawn) & pinned;
  while (b1)
  {
    s = BitBoardPop(&b1);
    b3 = LookupTableLineOfSight(l, BitBoardPeek(ChessBoardOur(cb, King)), s);
    moves = PAWN_ATTACKS(BitBoardAdd(EMPTY_BOARD, s), ChessBoardColor(cb)) & ChessBoardThem(cb);
    b2 = SINGLE_PUSH(BitBoardAdd(EMPTY_BOARD, s), ChessBoardColor(cb)) & ~ChessBoardAll(cb);
    moves |= b2;
    moves |= SINGLE_PUSH(b2 & ENPASSANT_RANK(ChessBoardColor(cb)), ChessBoardColor(cb)) & ~ChessBoardAll(cb);
    addMap(ms, moves & b3 & checkMask, BitBoardAdd(EMPTY_BOARD, s), Pawn);
  }

  // En passant map
  if (ChessBoardEnPassant(cb) != EMPTY_SQUARE)
  {
    b1 = PAWN_ATTACKS(BitBoardAdd(EMPTY_BOARD, ChessBoardEnPassant(cb)), (!ChessBoardColor(cb))) & ChessBoardOur(cb, Pawn);
    b2 = EMPTY_BOARD;
    b3 = BitBoardAdd(EMPTY_BOARD, ChessBoardEnPassant(cb));
    while (b1)
    {
      s = BitBoardPop(&b1);
      // Check pseudo-pinning for en passant
      if (LookupTableAttacks(l, BitBoardPeek(ChessBoardOur(cb, King)), Rook, ChessBoardAll(cb) & ~BitBoardAdd(SINGLE_PUSH(b3, (!ChessBoardColor(cb))), s)) &
          RANK_OF(BitBoardPeek(ChessBoardOur(cb, King))) & (ChessBoardTheir(cb, Rook) | ChessBoardTheir(cb, Queen)))
        continue;
      b2 |= BitBoardAdd(EMPTY_BOARD, s);
      if (b2 & pinned)
        b2 &= LookupTableLineOfSight(l, BitBoardPeek(ChessBoardOur(cb, King)), ChessBoardEnPassant(cb));
    }
    if (b2 != EMPTY_BOARD)
      addMap(ms, b3, b2, Pawn);
  }
}

int MoveSetCount(MoveSet *ms)
{
  int nodes = 0;
  for (int i = 0; i < ms->size; i++)
  {
    int offset = BitBoardCount(ms->maps[i].to) - BitBoardCount(ms->maps[i].from);
    BitBoard promotion = EMPTY_BOARD;
    if (ms->maps[i].type == Pawn)
      promotion |= ((BACK_RANK(White) | BACK_RANK(Black)) & ms->maps[i].to);
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
  ChessBoard *cb = ms->cb;

  // Save undo info
  m.enPassant = cb->enPassant;
  m.castling = cb->castling;

  // Pop a single square from each map to form the move
  Square fromSq = BitBoardPop(&ms->maps[i].from);
  Square toSq   = BitBoardPop(&ms->maps[i].to);
  m.from.square = fromSq;
  m.to.square   = toSq;
  m.to.type     = ms->maps[i].type;

  // If not surjective/bijective, put back excess squares
  if (offset > 0)
    ms->maps[i].from = BitBoardAdd(ms->maps[i].from, fromSq);
  else if (offset < 0)
    ms->maps[i].to = BitBoardAdd(ms->maps[i].to, toSq);

  // Handle pawn promotions: iterate piece types beyond Pawn
  if ((m.to.type == Pawn) && (BitBoardAdd(EMPTY_BOARD, toSq) & (BACK_RANK(White) | BACK_RANK(Black)))) {
    // First promotion popped: yield Knight
    if ((ms->prev.from.square != fromSq) || (ms->prev.to.square != toSq)) {
      m.to.type = Knight;
      ms->maps[i].from = BitBoardAdd(ms->maps[i].from, fromSq);
      ms->maps[i].to   = BitBoardAdd(ms->maps[i].to, toSq);
    } else {
      // Subsequent promotions: increment type
      Type prevType = ms->prev.to.type;
      m.to.type = (Type)(prevType + 1);
      if (prevType != Rook) {
        ms->maps[i].from = BitBoardAdd(ms->maps[i].from, fromSq);
        ms->maps[i].to   = BitBoardAdd(ms->maps[i].to, toSq);
      }
    }
  }

  // If this map is exhausted, shrink size
  if (ms->maps[i].to == EMPTY_BOARD)
    ms->size--;

  // Populate origin and captured pieces for undo
  m.from.type = cb->squares[fromSq];
  if ((m.from.type == Pawn) && (toSq == m.enPassant)) {
    m.captured.square = (cb->turn == White) ? toSq + EDGE_SIZE : toSq - EDGE_SIZE;
    m.captured.type   = Pawn;
  } else {
    m.captured.square = toSq;
    m.captured.type   = cb->squares[toSq];
  }

  // Record this move for promotion sequencing
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
  theirMoves |= pawnMoves(ChessBoardTheir(cb, Pawn), !ChessBoardColor(cb));
  for (int i = 0; i < next.size; i++)
  {
    if (next.maps[i].type > Knight)
      theirMoves |= next.maps[i].to;
  }

  // Consider our moves that could disrupt their king moves
  BitBoard kingRelevant = (LookupTableAttacks(l, BitBoardPeek(ChessBoardTheir(cb, King)), King, EMPTY_BOARD) & ~ChessBoardThem(cb)) |
                          BitBoardAdd(EMPTY_BOARD, BitBoardPeek(ChessBoardTheir(cb, King)));
  while (kingRelevant)
  {
    Square s1 = BitBoardPop(&kingRelevant);
    BitBoard ourPieces = ChessBoardUs(cb) & ~ChessBoardOur(cb, Pawn);
    while (ourPieces)
    {
      Square s2 = BitBoardPop(&ourPieces);
      Type t = ChessBoardSquare(cb, s2);
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
    BitBoard moves = pawnMoves(ChessBoardOur(cb, Pawn), ChessBoardColor(cb));
    canAttack[Pawn] |= moves & projection;
    isAttacking |= ChessBoardOur(cb, Pawn) & projection;
  }

  // Consider special moves that are annoying to calculate
  for (int i = 0; i < ms->size; i++)
  {
    Type t = ms->maps[i].type;
    int squareOffset = BitBoardPeek(ms->maps[i].to) - BitBoardPeek(ms->maps[i].from);

    if (t == Pawn)
    {
      if (squareOffset == 16 || squareOffset == -16) // Double pushes causing en passant
        special |= SINGLE_PUSH(SINGLE_PUSH(ms->maps[i].from, ChessBoardColor(cb)) & PAWN_ATTACKS(ChessBoardTheir(cb, Pawn), !ChessBoardColor(cb)), !ChessBoardColor(cb));
      if (ChessBoardEnPassant(cb) != EMPTY_SQUARE) // En passant
        special |= PAWN_ATTACKS(BitBoardAdd(EMPTY_BOARD, ChessBoardEnPassant(cb)), !ChessBoardColor(cb)) & ms->maps[i].from;
      special |= ms->maps[i].from & PROMOTION_RANK(ChessBoardColor(cb)); // Promotion
    }
    else if (t == King) // Castling
      castling = ms->maps[i].to & ~LookupTableAttacks(l, BitBoardPeek(ChessBoardOur(cb, King)), King, EMPTY_BOARD);
  }

  // Remove moves from ms that can be easily multiplied by the next moveset
  for (int i = 0; i < ms->size; i++)
  {
    Type t = ms->maps[i].type;
    int mapOffset = BitBoardCount(ms->maps[i].to) - BitBoardCount(ms->maps[i].from);

    if (mapOffset > 0) // Injective
    {
      if (!(ms->maps[i].from & (pinned | isAttacking | theirMoves)))
        ms->maps[i].to &= pinned | canAttack[t] | theirMoves | ChessBoardThem(cb) | castling;
    }
    else if (mapOffset == 0) // Bijective
    {
      int squareOffset = BitBoardPeek(ms->maps[i].to) - BitBoardPeek(ms->maps[i].from);
      if (squareOffset > 0)
      {
        ms->maps[i].to &= pinned | canAttack[t] | theirMoves | ChessBoardThem(cb) | ((pinned | isAttacking | theirMoves | special) << squareOffset);
        ms->maps[i].from = ms->maps[i].to >> squareOffset;
      }
      else // squareOffset < 0
      {
        ms->maps[i].to &= pinned | canAttack[t] | theirMoves | ChessBoardThem(cb) | ((pinned | isAttacking | theirMoves | special) >> -squareOffset);
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