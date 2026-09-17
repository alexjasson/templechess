#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include "MoveSet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Context for MoveSetMultiply algorithm
typedef struct
{
  MoveSet s;

  int kingSide, queenSide;
  BitBoard attacked, pinned, from, to[TYPE_SIZE]; // to[Empty] represents all types
  BitBoard kingDestinations;
  BitBoard changed, captured, theirKnights;

  // Their pawn moves
  BitBoard pawnLeft, pawnRight, pawnSingle, pawnDoubles, pawnDoubleFirst, pawnDoubleLast;

  // Their slider moves
  BitBoard sliderAttacks[MAPS_SIZE], sliderLines[MAPS_SIZE];
  Square sliderSquares[MAPS_SIZE];
  Type sliderTypes[MAPS_SIZE];
  int sliderCounts[MAPS_SIZE];
  int sliderCount;
} Context;

static void addMap(MoveSet *ms, BitBoard to, BitBoard from, Type type);

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
  BitBoard pinned, checking;
  ChessBoardCheckingAndPinned(l, cb, &checking, &pinned);
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
    BitBoard sqBit = BitBoardAdd(EMPTY_BOARD, s);
    moves = LookupTableAttacks(l, s, Knight, all) & notUsAndCheck;
    if (sqBit & pinned)
      moves &= LookupTableLineOfSight(l, kingSq, s);
    addMap(ms, moves, sqBit, Knight);
  }

  // Diagonal slider maps (bishops + queens)
  BitBoard diagSliders = ChessBoardOur(cb, Bishop) | ChessBoardOur(cb, Queen);
  while (diagSliders) {
    s = BitBoardPop(&diagSliders);
    BitBoard sqBit = BitBoardAdd(EMPTY_BOARD, s);
    moves = LookupTableAttacks(l, s, Bishop, all) & notUsAndCheck;
    if (sqBit & pinned)
      moves &= LookupTableLineOfSight(l, kingSq, s);
    addMap(ms, moves, sqBit, ChessBoardSquare(cb, s));
  }

  // Orthogonal slider maps (rooks + queens)
  BitBoard orthoSliders = ChessBoardOur(cb, Rook) | ChessBoardOur(cb, Queen);
  while (orthoSliders) {
    s = BitBoardPop(&orthoSliders);
    BitBoard sqBit = BitBoardAdd(EMPTY_BOARD, s);
    moves = LookupTableAttacks(l, s, Rook, all) & notUsAndCheck;
    if (sqBit & pinned)
      moves &= LookupTableLineOfSight(l, kingSq, s);
    addMap(ms, moves, sqBit, ChessBoardSquare(cb, s));
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
  if (ChessBoardEnPassant(cb) != EMPTY_SQUARE)
    addMap(ms, BitBoardAdd(0, ChessBoardEnPassant(cb)),
           ChessBoardEnPassantCapturers(l, cb), Pawn);
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

static inline Context getContext(LookupTable l, MoveSet *ms)
{
  Context ctx;
  if (ms->size == 0) {
    ctx.s.size = 0;
    return ctx;
  }
  ChessBoard *cb = ms->cb;
  // Cache hot values
  const Color color = ChessBoardColor(cb);
  const BitBoard us = ChessBoardUs(cb), them = ChessBoardThem(cb);
  const BitBoard ourKing = ChessBoardOur(cb, King);
  const BitBoard theirKing = ChessBoardTheir(cb, King);
  const BitBoard theirPawns = ChessBoardTheir(cb, Pawn);
  const Square kingSq = BitBoardPeek(theirKing);
  const BitBoard enemy = them & ~theirKing;  // Their pieces, excluding their king
  const BitBoard knights = ChessBoardOur(cb, Knight);
  const BitBoard diagSliders = ChessBoardOur(cb, Bishop) | ChessBoardOur(cb, Queen);
  const BitBoard orthoSliders = ChessBoardOur(cb, Rook) | ChessBoardOur(cb, Queen);

  ctx.attacked = PAWN_ATTACKS(ChessBoardOur(cb, Pawn), color) |
                      LookupTableAttacks(l, BitBoardPeek(ourKing), King, EMPTY_BOARD);
  ctx.pinned = EMPTY_BOARD;
  ctx.from = EMPTY_BOARD;
  memset(ctx.to, EMPTY_BOARD, sizeof(ctx.to));

  // Their relevant king squares
  ctx.kingDestinations = LookupTableAttacks(l, kingSq, King, EMPTY_BOARD) & ~them;
  BitBoard relevant = ctx.kingDestinations | theirKing;
  BitBoard missingRights = ~ChessBoardCastling(cb) & BACK_RANK(!color);
  ctx.kingSide = !(missingRights & KINGSIDE_CASTLING) && !(them & (theirKing << 1 | theirKing << 2));
  ctx.queenSide = !(missingRights & QUEENSIDE_CASTLING) && !(them & (theirKing >> 1 | theirKing >> 2 | theirKing >> 3));
  if (ctx.kingSide) {
    relevant |= theirKing << 1 | theirKing << 2;
    ctx.to[Empty] |= theirKing << 1 | theirKing << 2;
  }
  if (ctx.queenSide) {
    relevant |= theirKing >> 1 | theirKing >> 2;
    ctx.to[Empty] |= theirKing >> 1 | theirKing >> 2 | theirKing >> 3;
  }
  ctx.from |= ctx.to[Empty];

  // Our moves that could attack or unattack one of those squares
  ctx.to[Pawn] = PAWN_ATTACKS(relevant, !color);
  ctx.from |= ChessBoardOur(cb, Pawn) & ctx.to[Pawn];
  BitBoard adjacent = ((relevant & ~WEST_EDGE) >> 1) | ((relevant & ~EAST_EDGE) << 1);
  ctx.to[King] = adjacent | BitBoardShiftN(adjacent | relevant) | BitBoardShiftS(adjacent | relevant);
  relevant &= ~theirKing;
  Square sq = kingSq;
  int pins = 1;
  while (1) {
    if (knights) {
      BitBoard attacks = LookupTableAttacks(l, sq, Knight, EMPTY_BOARD);
      ctx.to[Knight] |= attacks;
      if (attacks & knights) ctx.attacked |= BitBoardAdd(EMPTY_BOARD, sq);
    }

    BitBoard bishops = EMPTY_BOARD, rooks = EMPTY_BOARD;
    if (diagSliders) {
      bishops = LookupTableAttacks(l, sq, Bishop, EMPTY_BOARD) & diagSliders;
      BitBoard enemyRay = LookupTableAttacks(l, sq, Bishop, enemy);
      if (pins) enemyRay = LookupTableAttacks(l, sq, Bishop, enemy & ~enemyRay);
      ctx.to[Bishop] |= LookupTableAttacks(l, sq, Bishop, us & ~diagSliders) & enemyRay;
    }
    if (orthoSliders) {
      rooks = LookupTableAttacks(l, sq, Rook, EMPTY_BOARD) & orthoSliders;
      BitBoard enemyRay = LookupTableAttacks(l, sq, Rook, enemy);
      if (pins) enemyRay = LookupTableAttacks(l, sq, Rook, enemy & ~enemyRay);
      ctx.to[Rook] |= LookupTableAttacks(l, sq, Rook, us & ~orthoSliders) & enemyRay;
    }

    BitBoard sliders = bishops | rooks;
    BitBoard rest = sliders & (sliders - 1);
    if (sliders && !(rest & (rest - 1))) {
      while (sliders) {
        BitBoard slider = sliders & -sliders;
        sliders &= sliders - 1;
        BitBoard between = LookupTableSquaresBetween(l, sq, BitBoardPeek(slider));
        BitBoard friendly = between & us, opposing = between & enemy;
        if (opposing && (!pins || (opposing & (opposing - 1))))
          continue;
        if (!friendly) {
          if (!opposing) ctx.attacked |= BitBoardAdd(EMPTY_BOARD, sq);
          else if (pins) ctx.pinned |= opposing;
          ctx.from |= slider;
          ctx.to[Empty] |= between;
        } else if (!(friendly & (friendly - 1))) {
          ctx.from |= friendly;
        }
      }
    } else if (sliders) {
      for (Type t = Bishop; t <= Rook; t++) {
        BitBoard typeSliders = (t == Bishop) ? bishops : rooks;
        if (typeSliders == EMPTY_BOARD) continue;
        BitBoard firstRay = LookupTableAttacks(l, sq, t, us);
        BitBoard first = firstRay & us;
        BitBoard secondRay = LookupTableAttacks(l, sq, t, us & ~first);
        BitBoard enemyRay = LookupTableAttacks(l, sq, t, enemy);
        BitBoard throughEnemy = pins ? LookupTableAttacks(l, sq, t, enemy & ~enemyRay) : enemyRay;
        BitBoard candidates = typeSliders & secondRay & throughEnemy;
        BitBoard visible = candidates & first;
        if (visible) {
          if (visible & enemyRay) ctx.attacked |= BitBoardAdd(EMPTY_BOARD, sq);
          if (pins && (visible & ~enemyRay))
            ctx.pinned |= enemy & enemyRay & LookupTableRays(l, sq, t, visible & ~enemyRay);
          ctx.from |= visible;
          ctx.to[Empty] |= firstRay & LookupTableRays(l, sq, t, visible);
        }
        BitBoard behind = candidates & ~first;
        if (behind)
          ctx.from |= first & LookupTableRays(l, sq, t, behind);
      }
    }

    if (pins && (ctx.attacked & theirKing)) {
      ctx.s.size = 0;
      return ctx;
    }
    if (relevant == EMPTY_BOARD) break;
    sq = BitBoardPop(&relevant);
    pins = 0;
  }
  ctx.to[Queen] = ctx.to[Bishop] | ctx.to[Rook];
  ctx.from |= (knights & ctx.to[Knight]) | (ourKing & ctx.to[King]);

  // Captures that change their replies, and our special moves
  ctx.to[Empty] |= them & (LookupTableAttacks(l, kingSq, King, EMPTY_BOARD) | theirKing | ChessBoardCastling(cb));
  ctx.to[Pawn] |= BACK_RANK(!color);                                   // Promotion
  ctx.to[Pawn] |= SINGLE_PUSH(SINGLE_PUSH(ChessBoardOur(cb, Pawn), color) & PAWN_ATTACKS(theirPawns, !color), color); // Double push causing ep
  if (ChessBoardEnPassant(cb) != EMPTY_SQUARE)
    ctx.to[Pawn] |= BitBoardAdd(EMPTY_BOARD, ChessBoardEnPassant(cb));  // En passant
  ctx.to[King] |= (ourKing << 2) | (ourKing >> 2);                      // Castling

  // Create the subset S
  MoveSet *s = &ctx.s;
  s->size = 0;
  s->prev.from.square = EMPTY_SQUARE;
  s->prev.from.type = Empty;
  s->prev.to.square = EMPTY_SQUARE;
  s->prev.to.type = Empty;
  BitBoard *changed = &ctx.changed;
  int remainingCount = 0;
  *changed = EMPTY_BOARD;
  for (int i = 0; i < ms->size; i++) {
    Map map = ms->maps[i];
    Map *m = &map;
    BitBoard origTo = m->to, origFrom = m->from;
    if (!(m->from & (m->from - 1))) {  // One origin
      if (!(m->from & ctx.from)) m->to &= ctx.to[Empty] | ctx.to[m->type];
    } else if (m->to & (m->to - 1)) {  // Bijective pawn map
      int shift = BitBoardPeek(m->to) - BitBoardPeek(m->from);
      BitBoard shifted = (shift >= 0) ? (ctx.from << shift) : (ctx.from >> -shift);
      m->to &= ctx.to[Empty] | ctx.to[m->type] | shifted;
      m->from = (shift >= 0) ? (m->to >> shift) : (m->to << -shift);
    }

    BitBoard removedTo = origTo & ~m->to;
    if (removedTo != EMPTY_BOARD) {
      BitBoard removedFrom = origFrom & ~m->from;
      BitBoard origins = (removedFrom != EMPTY_BOARD) ? removedFrom : origFrom;
      *changed |= removedTo | origins;
      if (origins != EMPTY_BOARD) {
        Map *accepted = &s->maps[s->size++];
        accepted->to = removedTo;
        accepted->from = origins;
        accepted->type = m->type;
      }
    }

    if (m->to) ms->maps[remainingCount++] = *m;
  }
  ctx.captured = ctx.changed & them;
  if (s->size == 0) return ctx;
  ms->size = remainingCount;
  return ctx;
}

static inline int cZero(LookupTable l, ChessBoard *cb, Context *ctx)
{
  const BitBoard them = ChessBoardThem(cb), all = ChessBoardAll(cb);
  const BitBoard pinned = ctx->pinned;
  const Square kingSq = BitBoardPeek(ChessBoardTheir(cb, King));
  const BitBoard theirKing = ChessBoardTheir(cb, King);
  int c0 = BitBoardCount(ctx->kingDestinations & ~ctx->attacked);
  if (ctx->kingSide && !((all | ctx->attacked) & (theirKing << 1 | theirKing << 2))) c0++;
  if (ctx->queenSide && !(all & (theirKing >> 1 | theirKing >> 2 | theirKing >> 3)) &&
      !(ctx->attacked & (theirKing >> 1 | theirKing >> 2))) c0++;

  // knights
  ctx->theirKnights = ChessBoardTheir(cb, Knight) & ~pinned;
  BitBoard knights = ctx->theirKnights;
  while (knights)
    c0 += BitBoardCount(LookupTableAttacks(l, BitBoardPop(&knights), Knight, EMPTY_BOARD) & ~them);

  // sliders
  ctx->sliderCount = 0;
  for (Type t = Bishop; t <= Rook; t++) {
    BitBoard pieces = ChessBoardTheir(cb, t) | ChessBoardTheir(cb, Queen);
    while (pieces) {
      Square s = BitBoardPop(&pieces);
      BitBoard piece = BitBoardAdd(EMPTY_BOARD, s);
      BitBoard line = (piece & pinned) ? LookupTableLineOfSight(l, kingSq, s) : ~(BitBoard)EMPTY_BOARD;
      BitBoard attacks = LookupTableAttacks(l, s, t, all) & line;
      int count = BitBoardCount(attacks & ~them);
      c0 += count;
      if (!((attacks | piece) & ctx->changed)) continue;
      int i = ctx->sliderCount++;
      ctx->sliderAttacks[i] = attacks;
      ctx->sliderLines[i] = line;
      ctx->sliderSquares[i] = s;
      ctx->sliderTypes[i] = t;
      ctx->sliderCounts[i] = count;
    }
  }

  // pawns
  const Color color = ChessBoardColor(cb);
  const BitBoard theirPawns = ChessBoardTheir(cb, Pawn);
  BitBoard pawns = theirPawns & ~pinned;
  ctx->pawnLeft = PAWN_ATTACKS_LEFT(pawns, !color);
  ctx->pawnRight = PAWN_ATTACKS_RIGHT(pawns, !color);
  ctx->pawnSingle = SINGLE_PUSH(pawns, !color);
  ctx->pawnDoubles = SINGLE_PUSH(ctx->pawnSingle & ENPASSANT_RANK(!color), !color);
  BitBoard pinnedPawns = theirPawns & pinned;
  while (pinnedPawns) {
    Square ps = BitBoardPop(&pinnedPawns);
    BitBoard pawn = BitBoardAdd(EMPTY_BOARD, ps), line = LookupTableLineOfSight(l, kingSq, ps);
    ctx->pawnLeft |= PAWN_ATTACKS_LEFT(pawn, !color) & line;
    ctx->pawnRight |= PAWN_ATTACKS_RIGHT(pawn, !color) & line;
    ctx->pawnSingle |= SINGLE_PUSH(pawn, !color) & line;
    ctx->pawnDoubles |= SINGLE_PUSH(SINGLE_PUSH(pawn, !color) & ENPASSANT_RANK(!color), !color) & line;
  }
  ctx->pawnDoubleFirst = SINGLE_PUSH(ctx->pawnDoubles & ~all, color);
  ctx->pawnDoubleLast = ctx->pawnDoubles & SINGLE_PUSH(~all, !color);
  const BitBoard us = ChessBoardUs(cb);
  BitBoard promotion = BACK_RANK(color);
  BitBoard capturesLeft = ctx->pawnLeft & us, capturesRight = ctx->pawnRight & us;
  BitBoard pushes = ctx->pawnSingle & ~all;
  return c0 + BitBoardCount(capturesLeft) + BitBoardCount(capturesRight) + BitBoardCount(pushes) +
         BitBoardCount(ctx->pawnDoubleLast & ~all) +
         3 * (BitBoardCount(capturesLeft & promotion) + BitBoardCount(capturesRight & promotion) +
              BitBoardCount(pushes & promotion));
}

static inline int deltaSliders(LookupTable l, ChessBoard *cb, const Context *ctx)
{
  const MoveSet *s = &ctx->s;
  const BitBoard captured = ctx->captured;
  const BitBoard them = ChessBoardThem(cb), all = ChessBoardAll(cb);
  int delta = 0;
  for (int j = 0; j < ctx->sliderCount; j++) {
    const Square sq = ctx->sliderSquares[j];
    const Type t = ctx->sliderTypes[j];
    const BitBoard piece = BitBoardAdd(EMPTY_BOARD, sq);
    const BitBoard line = ctx->sliderLines[j], attacks = ctx->sliderAttacks[j];
    const BitBoard mask = ~them & line;
    const int count = ctx->sliderCounts[j];

    for (int i = 0; i < s->size; i++) {
      Map m = s->maps[i];
      if (!((m.from | m.to) & (attacks | (piece & captured)))) continue;
      int bijective = (m.from & (m.from - 1)) != EMPTY_BOARD;
      int shift = BitBoardPeek(m.to) - BitBoardPeek(m.from);
      if (m.to & piece) {
        // captured slider
        delta -= count;
        m.to &= ~piece;
      }
      if (!m.to || !((m.from | m.to) & attacks)) continue;
      if (bijective) {
        BitBoard shifted = (shift >= 0) ? (attacks << shift) : (attacks >> -shift);
        m.to &= attacks | shifted;
      } else if (!(m.from & attacks)) {
        m.to &= attacks;
      }
      while (m.to) {
        Square target = BitBoardPop(&m.to);
        Square origin = bijective ? (Square)(target - shift) : BitBoardPeek(m.from);
        BitBoard targetBit = BitBoardAdd(EMPTY_BOARD, target);
        BitBoard occupancy = (all & ~BitBoardAdd(EMPTY_BOARD, origin)) | targetBit;
        BitBoard afterMask = mask | (targetBit & line);
        delta += BitBoardCount(LookupTableAttacks(l, sq, t, occupancy) & afterMask) - count;
      }
    }
  }
  return delta;
}

static inline int deltaPawns(LookupTable l, ChessBoard *cb, const Context *ctx)
{
  const MoveSet *s = &ctx->s;
  const BitBoard pinned = ctx->pinned, changed = ctx->changed;
  const Color color = ChessBoardColor(cb);
  const BitBoard them = ChessBoardThem(cb), theirPawns = ChessBoardTheir(cb, Pawn);
  const BitBoard us = ChessBoardUs(cb), all = ChessBoardAll(cb);
  const Square kingSq = BitBoardPeek(ChessBoardTheir(cb, King));
  const BitBoard promotion = BACK_RANK(color);
  int delta = 0;
  BitBoard pawnSquares = ctx->pawnLeft | ctx->pawnRight | ctx->pawnSingle |
                         ctx->pawnDoubles | theirPawns;
  if (!(changed & pawnSquares)) return delta;

  BitBoard pushPaths = ctx->pawnDoubleFirst | ctx->pawnDoubleLast;
  BitBoard blockedPushes = ctx->pawnSingle | pushPaths;
  BitBoard sharedPushes = ctx->pawnSingle & pushPaths;
  BitBoard promotionSquares = (ctx->pawnLeft | ctx->pawnRight | ctx->pawnSingle) & promotion;
  for (int i = 0; i < s->size; i++) {
    Map m = s->maps[i];
    if (!((m.from | m.to) & pawnSquares)) continue;
    int count = BitBoardCount(m.to);
    int bijective = (m.from & (m.from - 1)) != EMPTY_BOARD;
    int originWeight = bijective ? 1 : count;
    BitBoard emptyTargets = m.to & ~them;
    delta += BitBoardCount(m.to & ctx->pawnLeft) + BitBoardCount(m.to & ctx->pawnRight) -
             BitBoardCount(emptyTargets & blockedPushes) -
             BitBoardCount(emptyTargets & sharedPushes) -
             originWeight * (BitBoardCount(m.from & ctx->pawnLeft) +
                             BitBoardCount(m.from & ctx->pawnRight) -
                             BitBoardCount(m.from & blockedPushes) -
                             BitBoardCount(m.from & sharedPushes));
    // Promotion changes
    if ((m.from | m.to) & promotionSquares) {
      BitBoard targetPromotions = m.to & promotion;
      BitBoard originPromotions = m.from & promotion;
      delta += 3 * (BitBoardCount(targetPromotions & ctx->pawnLeft) +
                    BitBoardCount(targetPromotions & ctx->pawnRight) -
                    BitBoardCount(targetPromotions & ~them & ctx->pawnSingle) -
                    originWeight * (BitBoardCount(originPromotions & ctx->pawnLeft) +
                                    BitBoardCount(originPromotions & ctx->pawnRight) -
                                    BitBoardCount(originPromotions & ctx->pawnSingle)));
    }

    // Double push changes
    if (m.from & (ctx->pawnDoubles | SINGLE_PUSH(ctx->pawnDoubles, color))) {
      int shift = BitBoardPeek(m.to) - BitBoardPeek(m.from);
      int pushShift = (color == White) ? -EDGE_SIZE : EDGE_SIZE;
      BitBoard swaps = EMPTY_BOARD;
      if (!bijective || shift == -pushShift)
        swaps |= SINGLE_PUSH(m.from, !color) & ctx->pawnDoubles;
      if (!bijective || shift == pushShift)
        swaps |= SINGLE_PUSH(m.from & ctx->pawnDoubles, color);
      delta -= BitBoardCount(emptyTargets & swaps);
    }

    // Captured pawns
    int lost = 0;
    BitBoard capturedPawns = m.to & theirPawns;
    int shift = capturedPawns ? BitBoardPeek(m.to) - BitBoardPeek(m.from) : 0;
    while (capturedPawns) {
      Square target = BitBoardPop(&capturedPawns);
      Square origin = bijective ? (Square)(target - shift) : BitBoardPeek(m.from);
      BitBoard pawn = BitBoardAdd(EMPTY_BOARD, target), fromBit = BitBoardAdd(EMPTY_BOARD, origin);
      BitBoard occupancy = (all & ~fromBit) | pawn;
      BitBoard ours = (us & ~fromBit) | pawn;
      BitBoard line = (pawn & pinned) ? LookupTableLineOfSight(l, kingSq, target) : ~(BitBoard)EMPTY_BOARD;
      BitBoard captures = PAWN_ATTACKS(pawn, !color) & ours & line;
      BitBoard push = SINGLE_PUSH(pawn, !color) & ~occupancy;
      BitBoard twice = SINGLE_PUSH(push & ENPASSANT_RANK(!color), !color) & ~occupancy & line;
      push &= line;
      lost += BitBoardCount(captures) + BitBoardCount(push) + BitBoardCount(twice) +
              3 * BitBoardCount((captures | push) & promotion);
    }
    delta -= lost;
  }
  return delta;
}

static inline int deltaKnights(LookupTable l, ChessBoard *cb, const Context *ctx)
{
  const MoveSet *s = &ctx->s;
  const BitBoard them = ChessBoardThem(cb);
  const BitBoard theirKnights = ctx->theirKnights;
  int delta = 0;
  if (ctx->captured && theirKnights) {
    for (int i = 0; i < s->size; i++) {
      BitBoard captures = s->maps[i].to & them;
      while (captures) {
        Square sq = BitBoardPop(&captures);
        BitBoard attacks = LookupTableAttacks(l, sq, Knight, EMPTY_BOARD);
        delta += BitBoardCount(attacks & theirKnights);
        if (BitBoardAdd(EMPTY_BOARD, sq) & theirKnights)
          // captured knight
          delta -= BitBoardCount(attacks & ~them);
      }
    }
  }
  return delta;
}

/*
 * This algorithm can count specific edges at "depth 2" in the tree of moves. It returns
 * the number of edges that would've been searched had that edge been explored to "depth 1".
 * It returns |S| * C_0 + sum_{m in S}(delta_sl(m) + delta_pa(m) + delta_kn(m)).
 * |S| is the size of the subset removed from the given move set. C_0 is the opponents 
 * move count before applying any move in S, excluding enpassant. The delta terms are 
 * the amount that each move in S changes the reply count away from C_0.
 * Found by: Alex Jasson, GPT6
 */
int MoveSetMultiply(LookupTable l, MoveSet *ms)
{
  ChessBoard *cb = ms->cb;
  Context ctx = getContext(l, ms);
  int sSize = MoveSetCount(&ctx.s);
  if (!sSize) return 0;

  int c0 = cZero(l, cb, &ctx);
  int deltaSl = deltaSliders(l, cb, &ctx);
  int deltaPa = deltaPawns(l, cb, &ctx);
  int deltaKn = deltaKnights(l, cb, &ctx);

  return sSize * c0 + deltaSl + deltaPa + deltaKn;
}
