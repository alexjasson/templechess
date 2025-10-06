#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include "MoveSet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  ms.prev.from.square = EMPTY_SQUARE;
  ms.prev.from.type = Empty;
  ms.prev.to.square = EMPTY_SQUARE;
  ms.prev.to.type = Empty;
  return ms;
}

void MoveSetFill(LookupTable l, ChessBoard *cb, MoveSet *ms)
{
  ms->cb = cb;
  Square s;
  BitBoard pinned, checking, attacked, checkMask, moves, b1, b2, b3;
  attacked = ChessBoardAttacked(l, cb);
  checking = ChessBoardChecking(l, cb);
  pinned = ChessBoardPinned(l, cb);

  // Cache hot values
  const BitBoard us    = ChessBoardUs(cb);
  const BitBoard them  = ChessBoardThem(cb);
  const BitBoard all   = ChessBoardAll(cb);
  const BitBoard kingB = ChessBoardOur(cb, King);
  const Square kingSq  = BitBoardPeek(kingB);
  const int color      = ChessBoardColor(cb);

  // Determine check mask
  int numChecks = BitBoardCount(checking);
  if (numChecks == 0) {
    checkMask = ~EMPTY_BOARD;
  } else if (numChecks == 1) {
    Square cs = BitBoardPeek(checking);
    checkMask = BitBoardAdd(EMPTY_BOARD, cs) |
                LookupTableSquaresBetween(l, kingSq, cs);
  } else {
    checkMask = EMPTY_BOARD;
  }

  // King map
  moves = LookupTableAttacks(l, kingSq, King, EMPTY_BOARD) & ~us & ~attacked;
  if ((~checkMask) == EMPTY_BOARD) {
    // Kingside: check rights and interior squares f/g
    int clear = (((attacked & ATTACK_MASK) | (all & OCCUPANCY_MASK)) &
                 (KINGSIDE & ~KINGSIDE_CASTLING) & BACK_RANK(color)) == EMPTY_BOARD;
    if (ChessBoardKingSide(cb) && clear)
      moves |= kingB << 2;
    // Queenside: check rights and interior squares b/c/d
    clear = (((attacked & ATTACK_MASK) | (all & OCCUPANCY_MASK)) &
             (QUEENSIDE & ~QUEENSIDE_CASTLING) & BACK_RANK(color)) == EMPTY_BOARD;
    if (ChessBoardQueenSide(cb) && clear)
      moves |= kingB >> 2;
  }
  addMap(ms, moves, kingB, King);

  // If double-check, return early
  if (numChecks == 2) return;

  // Piece maps
  b1 = us & ~(ChessBoardOur(cb, Pawn) | kingB);
  while (b1)
  {
    s = BitBoardPop(&b1);
    moves = LookupTableAttacks(l, s, ChessBoardSquare(cb, s), all) & ~us & checkMask;
    if (BitBoardAdd(EMPTY_BOARD, s) & pinned)
      moves &= LookupTableLineOfSight(l, kingSq, s);
    addMap(ms, moves, BitBoardAdd(EMPTY_BOARD, s), ChessBoardSquare(cb, s));
  }

  // Pawn maps
  b1 = ChessBoardOur(cb, Pawn) & ~pinned;
  moves = PAWN_ATTACKS_LEFT(b1, color) & them & checkMask;
  addMap(ms, moves, PAWN_ATTACKS_LEFT(moves, (!color)), Pawn);
  moves = PAWN_ATTACKS_RIGHT(b1, color) & them & checkMask;
  addMap(ms, moves, PAWN_ATTACKS_RIGHT(moves, (!color)), Pawn);
  b2 = SINGLE_PUSH(b1, color) & ~all;
  moves = b2 & checkMask;
  addMap(ms, moves, SINGLE_PUSH(moves, (!color)), Pawn);
  moves = SINGLE_PUSH(b2 & ENPASSANT_RANK(color), color) & ~all & checkMask;
  addMap(ms, moves, DOUBLE_PUSH(moves, (!color)), Pawn);
  b1 = ChessBoardOur(cb, Pawn) & pinned;
  while (b1)
  {
    s = BitBoardPop(&b1);
    b3 = LookupTableLineOfSight(l, kingSq, s);
    moves = PAWN_ATTACKS(BitBoardAdd(EMPTY_BOARD, s), color) & them;
    b2 = SINGLE_PUSH(BitBoardAdd(EMPTY_BOARD, s), color) & ~all;
    moves |= b2;
    moves |= SINGLE_PUSH(b2 & ENPASSANT_RANK(color), color) & ~all;
    addMap(ms, moves & b3 & checkMask, BitBoardAdd(EMPTY_BOARD, s), Pawn);
  }

  // En passant map
  if (ChessBoardEnPassant(cb) != EMPTY_SQUARE)
  {
    b1 = PAWN_ATTACKS(BitBoardAdd(EMPTY_BOARD, ChessBoardEnPassant(cb)), (!color)) & ChessBoardOur(cb, Pawn);
    b2 = EMPTY_BOARD;
    b3 = BitBoardAdd(EMPTY_BOARD, ChessBoardEnPassant(cb));
    while (b1)
    {
      s = BitBoardPop(&b1);
      // Check pseudo-pinning for en passant
      if (LookupTableAttacks(l, kingSq, Rook, all &
          ~BitBoardAdd(SINGLE_PUSH(b3, (!color)), s)) &
          RANK_OF(kingSq) & (ChessBoardTheir(cb, Rook) | ChessBoardTheir(cb, Queen)))
        continue;
      b2 |= BitBoardAdd(EMPTY_BOARD, s);
      if (b2 & pinned)
        b2 &= LookupTableLineOfSight(l, kingSq, ChessBoardEnPassant(cb));
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
  m.enPassant = ChessBoardEnPassant(cb);
  m.castling  = ChessBoardCastling(cb);

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
  m.from.type = ChessBoardSquare(cb, fromSq);
  if ((m.from.type == Pawn) && (toSq == m.enPassant)) {
    m.captured.square = (ChessBoardColor(cb) == White) ? toSq + EDGE_SIZE : toSq - EDGE_SIZE;
    m.captured.type   = Pawn;
  } else {
    m.captured.square = toSq;
    m.captured.type   = ChessBoardSquare(cb, toSq);
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

/*
 * This algorithm can count specific edges at "depth 2" in the tree of moves. It returns
 * the number of edges that would've been searched had that edge been explored to "depth 1".
 * Found by: Alex Jasson
 */
int MoveSetMultiply(LookupTable l, MoveSet *ms)
{
  ChessBoard *curr = ms->cb;
  ChessBoard flip = ChessBoardFlip(curr);
  MoveSet next = MoveSetNew(); // Next set of moves
  MoveSet removed = MoveSetNew(); // Moves that will be removed from ms
  BitBoard from = EMPTY_BOARD; // 'from' squares to be removed
  BitBoard to[TYPE_SIZE];  // Type specific 'to' squares to be removed - Use empty piece to represent all pieces
  memset(to, EMPTY_BOARD, sizeof(to));
  MoveSetFill(l, &flip, &next);

  // Cache hot values
  const BitBoard us = ChessBoardUs(curr);
  const BitBoard them = ChessBoardThem(curr);
  const BitBoard ourPawns = ChessBoardOur(curr, Pawn);
  const BitBoard ourKing = ChessBoardOur(curr, King);
  const BitBoard theirPawns = ChessBoardTheir(curr, Pawn);
  const BitBoard theirKing = ChessBoardTheir(curr, King);
  const int color = ChessBoardColor(curr);

  // Consider our moves that could disrupt their moves (either captures or blocking)
  BitBoard theirMoves = pawnMoves(theirPawns, !color);
  for (int i = 0; i < next.size; i++) {
    if (next.maps[i].type < Bishop) continue;
    theirMoves |= next.maps[i].to;
  }
  to[Empty] |= theirMoves | them;
  from |= theirMoves;

  // Consider our moves that could disrupt their king moves
  BitBoard kingRelevant = (LookupTableAttacks(l, BitBoardPeek(theirKing), King, EMPTY_BOARD) & ~them) | theirKing;
  BitBoard projection = PAWN_ATTACKS(kingRelevant, !color);
  BitBoard moves = pawnMoves(ourPawns, color);
  to[Pawn] |= moves & projection; // To squares that would attack relevant king squares
  from |= ourPawns & projection; // From squares that are attacking relevant king squares
  while (kingRelevant)
  {
    Square s1 = BitBoardPop(&kingRelevant);
    BitBoard ourPieces = us & ~ourPawns;
    while (ourPieces)
    {
      Square s2 = BitBoardPop(&ourPieces);
      Type t = ChessBoardSquare(curr, s2);
      BitBoard piece = BitBoardAdd(EMPTY_BOARD, s2);
      BitBoard projection = LookupTableAttacks(l, s1, t, EMPTY_BOARD);
      BitBoard moves = LookupTableAttacks(l, s2, t, EMPTY_BOARD);
      to[t] |= moves & projection; // To squares that would attack relevant king squares
      from |= piece & projection; // From squares that are attacking relevant king squares
      if (piece & projection) {
        BitBoard pinned = LookupTableSquaresBetween(l, s1, s2); // These squares are "pinned" to relevant king squares
        to[Empty] |= pinned;
        from |= pinned;
      }
    }
  }

  // Consider special moves
  to[Pawn] |= SINGLE_PUSH(SINGLE_PUSH(ourPawns, color) & PAWN_ATTACKS(theirPawns, !color), color); // Double push causing ep
  if (ChessBoardEnPassant(curr) != EMPTY_SQUARE) // En passant
    to[Pawn] |= BitBoardAdd(EMPTY_BOARD, ChessBoardEnPassant(curr));
  to[Pawn] |= BACK_RANK(!color); // Promotion
  to[King] |= ourKing >> 2 | ourKing << 2; // Castling

  // Remove moves from ms and put them in removed
  for (int i = 0; i < ms->size;) {
    Map *m = &ms->maps[i];
    Type t = m->type;
    BitBoard origTo = m->to;
    BitBoard origFrom = m->from;
    int mapOffset = BitBoardCount(m->to) - BitBoardCount(m->from);

    if (mapOffset > 0) {  // Injective
      if ((m->from & from) == 0)
        m->to &= (to[Empty] | to[t]);
    } else if (mapOffset == 0) {  // Bijective
      int squareOffset = BitBoardPeek(m->to) - BitBoardPeek(m->from);
      BitBoard fromShifted = (squareOffset >= 0) ? (from << squareOffset) : (from >> -squareOffset);
      m->to   &= (to[Empty] | to[t] | fromShifted);
      m->from  = (squareOffset >= 0) ? (m->to >> squareOffset) : (m->to << -squareOffset);
    }

    BitBoard removedTo = origTo & ~m->to;
    BitBoard removedFrom = origFrom & ~m->from;
    if (removedTo != EMPTY_BOARD || removedFrom != EMPTY_BOARD) {
      addMap(&removed, removedTo != EMPTY_BOARD ? removedTo : origTo,
                       removedFrom != EMPTY_BOARD ? removedFrom : origFrom, t);
    }

    // Remove map from ms if empty; otherwise advance
    if ((m->to == EMPTY_BOARD) || (m->from == EMPTY_BOARD))
      ms->maps[i] = ms->maps[--ms->size];
    else
      i++;
  }

  return MoveSetCount(&removed) * MoveSetCount(&next);
}