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
  BitBoard pinned   = ChessBoardPinned(l, cb);
  BitBoard checking = ChessBoardChecking(l, cb);
  BitBoard attacked = ChessBoardAttacked(l, cb);

  // Cache hot values
  const BitBoard us    = ChessBoardUs(cb);
  const BitBoard them  = ChessBoardThem(cb);
  const BitBoard all   = ChessBoardAll(cb);
  const BitBoard kingB = ChessBoardOur(cb, King);
  const Square  kingSq = BitBoardPeek(kingB);
  const int     color  = ChessBoardColor(cb);

  // Determine check mask
  BitBoard checkMask;
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
  BitBoard moves = LookupTableAttacks(l, kingSq, King, EMPTY_BOARD) & ~us & ~attacked;
  if ((~checkMask) == EMPTY_BOARD) {
    // Kingside: check rights and interior squares f/g
    int clear = (((attacked & ATTACK_MASK) | (all & OCCUPANCY_MASK)) &
                 (KINGSIDE & ~KINGSIDE_CASTLING) & BACK_RANK(color)) == EMPTY_BOARD;
    if (ChessBoardKingSide(cb) && clear)
      moves |= (kingB << 2);
    // Queenside: check rights and interior squares b/c/d
    clear = (((attacked & ATTACK_MASK) | (all & OCCUPANCY_MASK)) &
             (QUEENSIDE & ~QUEENSIDE_CASTLING) & BACK_RANK(color)) == EMPTY_BOARD;
    if (ChessBoardQueenSide(cb) && clear)
      moves |= (kingB >> 2);
  }
  addMap(ms, moves, kingB, King);

  // If double-check, return early
  if (numChecks == 2) return;

  const BitBoard notUsAndCheck = ~us & checkMask;

  // Knight maps
  BitBoard piecesKnight = ChessBoardOur(cb, Knight);
  while (piecesKnight) {
    s = BitBoardPop(&piecesKnight);
    moves = LookupTableAttacks(l, s, Knight, all) & notUsAndCheck;
    if (BitBoardAdd(EMPTY_BOARD, s) & pinned)
      moves &= LookupTableLineOfSight(l, kingSq, s);
    addMap(ms, moves, BitBoardAdd(EMPTY_BOARD, s), Knight);
  }

  // Bishop maps
  BitBoard piecesBishop = ChessBoardOur(cb, Bishop);
  while (piecesBishop) {
    s = BitBoardPop(&piecesBishop);
    moves = LookupTableAttacks(l, s, Bishop, all) & notUsAndCheck;
    if (BitBoardAdd(EMPTY_BOARD, s) & pinned)
      moves &= LookupTableLineOfSight(l, kingSq, s);
    addMap(ms, moves, BitBoardAdd(EMPTY_BOARD, s), Bishop);
  }

  // Rook maps
  BitBoard piecesRook = ChessBoardOur(cb, Rook);
  while (piecesRook) {
    s = BitBoardPop(&piecesRook);
    moves = LookupTableAttacks(l, s, Rook, all) & notUsAndCheck;
    if (BitBoardAdd(EMPTY_BOARD, s) & pinned)
      moves &= LookupTableLineOfSight(l, kingSq, s);
    addMap(ms, moves, BitBoardAdd(EMPTY_BOARD, s), Rook);
  }

  // Queen maps
  BitBoard piecesQueen = ChessBoardOur(cb, Queen);
  while (piecesQueen) {
    s = BitBoardPop(&piecesQueen);
    moves = LookupTableAttacks(l, s, Queen, all) & notUsAndCheck;
    if (BitBoardAdd(EMPTY_BOARD, s) & pinned)
      moves &= LookupTableLineOfSight(l, kingSq, s);
    addMap(ms, moves, BitBoardAdd(EMPTY_BOARD, s), Queen);
  }

  // Pawn maps
  BitBoard b1, b2, b3;
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
  while (b1) {
    s  = BitBoardPop(&b1);
    b3 = LookupTableLineOfSight(l, kingSq, s);
    moves  = PAWN_ATTACKS(BitBoardAdd(EMPTY_BOARD, s), color) & them;
    b2     = SINGLE_PUSH(BitBoardAdd(EMPTY_BOARD, s), color) & ~all;
    moves |= b2;
    moves |= SINGLE_PUSH(b2 & ENPASSANT_RANK(color), color) & ~all;
    addMap(ms, moves & b3 & checkMask, BitBoardAdd(EMPTY_BOARD, s), Pawn);
  }

  // En passant map
  if (ChessBoardEnPassant(cb) != EMPTY_SQUARE) {
    b3 = BitBoardAdd(EMPTY_BOARD, ChessBoardEnPassant(cb));
    b1 = PAWN_ATTACKS(b3, (!color)) & ChessBoardOur(cb, Pawn);
    b2 = EMPTY_BOARD;
    while (b1) {
      s = BitBoardPop(&b1);
      // Pseudo-pinning check for en passant
      if (LookupTableAttacks(l, kingSq, Rook,
            all & ~BitBoardAdd(SINGLE_PUSH(b3, (!color)), s)) &
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
  MoveSet next = MoveSetNew();    // Next set of moves
  MoveSet removed = MoveSetNew(); // Moves that will be removed from ms
  BitBoard from = EMPTY_BOARD;    // 'from' squares to be removed
  BitBoard to[TYPE_SIZE];         // Type-specific 'to' squares (Empty index = all)
  memset(to, EMPTY_BOARD, sizeof(to));
  MoveSetFill(l, &flip, &next);

  // Cache hot values
  const BitBoard them       = ChessBoardThem(curr);
  const BitBoard ourPawns   = ChessBoardOur(curr, Pawn);
  const BitBoard ourKing    = ChessBoardOur(curr, King);
  const BitBoard theirPawns = ChessBoardTheir(curr, Pawn);
  const BitBoard theirKing  = ChessBoardTheir(curr, King);
  const int      color      = ChessBoardColor(curr);

  // Per-type piece sets
  BitBoard piecesKnight = ChessBoardOur(curr, Knight);
  BitBoard piecesBishop = ChessBoardOur(curr, Bishop);
  BitBoard piecesRook   = ChessBoardOur(curr, Rook);
  BitBoard piecesQueen  = ChessBoardOur(curr, Queen);
  BitBoard piecesKing   = ourKing;

  // 1) Our moves that could disrupt their moves (captures or blocking)
  BitBoard theirMoves = pawnMoves(theirPawns, !color);
  for (int i = 0; i < next.size; i++) {
    if (next.maps[i].type < Bishop) continue;
    theirMoves |= next.maps[i].to;
  }
  to[Empty] |= theirMoves | them;
  from      |= theirMoves;

  // 2) Our moves that could disrupt their king moves
  BitBoard kingRelevant = (LookupTableAttacks(l, BitBoardPeek(theirKing), King, EMPTY_BOARD) & ~them) | theirKing;

  // Pawns
  BitBoard projection    = PAWN_ATTACKS(kingRelevant, !color);
  BitBoard pawnMovesUs = pawnMoves(ourPawns, color);
  to[Pawn] |= (pawnMovesUs & projection);  // to squares that would attack relevant king squares
  from     |= (ourPawns   & projection);   // from squares already attacking relevant king squares

  while (kingRelevant) {
    Square s1 = BitBoardPop(&kingRelevant);
    // Knights
    BitBoard bb = piecesKnight;
    projection = LookupTableAttacks(l, s1, Knight, EMPTY_BOARD);
    while (bb) {
      Square s2   = BitBoardPop(&bb);
      BitBoard piece = BitBoardAdd(EMPTY_BOARD, s2);
      BitBoard moves = LookupTableAttacks(l, s2, Knight, EMPTY_BOARD);
      to[Knight] |= (moves & projection);
      from       |= (piece & projection);
    }
    // Bishops
    projection = LookupTableAttacks(l, s1, Bishop, EMPTY_BOARD);
    bb = piecesBishop;
    while (bb) {
      Square s2   = BitBoardPop(&bb);
      BitBoard piece = BitBoardAdd(EMPTY_BOARD, s2);
      BitBoard moves = LookupTableAttacks(l, s2, Bishop, EMPTY_BOARD);
      to[Bishop] |= (moves & projection);
      from       |= (piece & projection);
      if (piece & projection) {
        BitBoard pinned = LookupTableSquaresBetween(l, s1, s2);
        to[Empty] |= pinned;
        from      |= pinned;
      }
    }
    // Rooks
    projection = LookupTableAttacks(l, s1, Rook, EMPTY_BOARD);
    bb = piecesRook;
    while (bb) {
      Square s2   = BitBoardPop(&bb);
      BitBoard piece = BitBoardAdd(EMPTY_BOARD, s2);
      BitBoard moves = LookupTableAttacks(l, s2, Rook, EMPTY_BOARD);
      to[Rook] |= (moves & projection);
      from     |= (piece & projection);
      if (piece & projection) {
        BitBoard pinned = LookupTableSquaresBetween(l, s1, s2);
        to[Empty] |= pinned;
        from      |= pinned;
      }
    }
    // Queens
    projection = LookupTableAttacks(l, s1, Queen, EMPTY_BOARD);
    bb = piecesQueen;
    while (bb) {
      Square s2   = BitBoardPop(&bb);
      BitBoard piece = BitBoardAdd(EMPTY_BOARD, s2);
      BitBoard moves = LookupTableAttacks(l, s2, Queen, EMPTY_BOARD);
      to[Queen] |= (moves & projection);
      from      |= (piece & projection);
      if (piece & projection) {
        BitBoard pinned = LookupTableSquaresBetween(l, s1, s2);
        to[Empty] |= pinned;
        from      |= pinned;
      }
    }
    // Kings
    projection = LookupTableAttacks(l, s1, King, EMPTY_BOARD);
    bb = piecesKing;
    while (bb) {
      Square s2   = BitBoardPop(&bb);
      BitBoard piece = BitBoardAdd(EMPTY_BOARD, s2);
      BitBoard moves = LookupTableAttacks(l, s2, King, EMPTY_BOARD);
      to[King] |= (moves & projection);
      from     |= (piece & projection);
    }
  }

  // 3) Special moves
  to[Pawn] |= SINGLE_PUSH(SINGLE_PUSH(ourPawns, color) & PAWN_ATTACKS(theirPawns, !color), color); // Double push causing ep
  if (ChessBoardEnPassant(curr) != EMPTY_SQUARE)
    to[Pawn] |= BitBoardAdd(EMPTY_BOARD, ChessBoardEnPassant(curr)); // En passant
  to[Pawn] |= BACK_RANK(!color);                                     // Promotion
  to[King] |= (ourKing >> 2) | (ourKing << 2);                       // Castling

  // 4) Remove moves from ms and put them in removed
  for (int i = 0; i < ms->size;) {
    Map *m = &ms->maps[i];
    Type t = m->type;
    BitBoard origTo   = m->to;
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

    BitBoard removedTo   = origTo   & ~m->to;
    BitBoard removedFrom = origFrom & ~m->from;
    if (removedTo != EMPTY_BOARD || removedFrom != EMPTY_BOARD) {
      addMap(&removed, removedTo   != EMPTY_BOARD ? removedTo   : origTo,
             removedFrom != EMPTY_BOARD ? removedFrom : origFrom, t);
    }

    if ((m->to == EMPTY_BOARD) || (m->from == EMPTY_BOARD))
      ms->maps[i] = ms->maps[--ms->size];
    else
      i++;
  }

  return MoveSetCount(&removed) * MoveSetCount(&next);
}
