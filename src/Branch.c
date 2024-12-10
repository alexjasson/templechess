#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include "Branch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OUR(t) (cb->pieces[GET_PIECE(t, cb->turn)])    // Bitboard representing our pieces of type t
#define THEIR(t) (cb->pieces[GET_PIECE(t, !cb->turn)]) // Bitboard representing their pieces of type t
#define ALL (~cb->pieces[EMPTY_PIECE])                 // Bitboard of all the pieces

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

#define GET_RANK(s) (SOUTH_EDGE >> (EDGE_SIZE * (EDGE_SIZE - BitBoardGetRank(s) - 1)))
#define ENPASSANT_RANK(c) (BitBoard) SOUTH_EDGE >> (EDGE_SIZE * ((c * 3) + 2))
#define PROMOTING_RANK(c) (BitBoard)((c == White) ? NORTH_EDGE : SOUTH_EDGE)
#define BACK_RANK(c) (BitBoard)((c == White) ? SOUTH_EDGE : NORTH_EDGE)

static BitBoard getAttackedSquares(LookupTable l, ChessBoard *cb, BitBoard them);
static BitBoard getCheckingPieces(LookupTable l, ChessBoard *cb, BitBoard them, BitBoard *pinned);

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
  BitBoard us, them, pinned, checking, attacked, checkMask, moves, b1, b2, b3;
  us = OUR(Pawn) | OUR(Knight) | OUR(Bishop) | OUR(Rook) | OUR(Queen) | OUR(King);
  them = ALL & ~us;

  attacked = getAttackedSquares(l, cb, them);
  checking = getCheckingPieces(l, cb, them, &pinned);
  checkMask = ~EMPTY_BOARD;
  while (checking)
  {
    s = BitBoardPopLSB(&checking);
    checkMask &= (BitBoardSetBit(EMPTY_BOARD, s) | LookupTableGetSquaresBetween(l, BitBoardGetLSB(OUR(King)), s));
  }

  // King moves
  moves = LookupTableAttacks(l, BitBoardGetLSB(OUR(King)), King, EMPTY_BOARD) & ~us & ~attacked;
  b1 = (cb->castling | (attacked & ATTACK_MASK) | (ALL & OCCUPANCY_MASK)) & BACK_RANK(cb->turn);
  if ((b1 & KINGSIDE) == (KINGSIDE_CASTLING & BACK_RANK(cb->turn)) && (~checkMask == EMPTY_BOARD))
    moves |= OUR(King) << 2;
  if ((b1 & QUEENSIDE) == (QUEENSIDE_CASTLING & BACK_RANK(cb->turn)) && (~checkMask == EMPTY_BOARD))
    moves |= OUR(King) >> 2;
  b[size++] = BranchNew(moves, OUR(King), GET_PIECE(King, cb->turn));

  // Piece moves
  b1 = us & ~(OUR(Pawn) | OUR(King));
  while (b1)
  {
    s = BitBoardPopLSB(&b1);
    moves = LookupTableAttacks(l, s, GET_TYPE(cb->squares[s]), ALL) & ~us & checkMask;
    if (BitBoardSetBit(EMPTY_BOARD, s) & pinned)
    {
      moves &= LookupTableGetLineOfSight(l, BitBoardGetLSB(OUR(King)), s);
    }
    b[size++] = BranchNew(moves, BitBoardSetBit(EMPTY_BOARD, s), cb->squares[s]);
  }

  // Pawn moves
  b1 = OUR(Pawn);

  // PAWN_ATTACKS_LEFT
  moves = PAWN_ATTACKS_LEFT(b1, cb->turn) & them & checkMask;
  b[size++] = BranchNew(moves, PAWN_ATTACKS_LEFT(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));

  // PAWN_ATTACKS_RIGHT
  moves = PAWN_ATTACKS_RIGHT(b1, cb->turn) & them & checkMask;
  b[size++] = BranchNew(moves, PAWN_ATTACKS_RIGHT(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));

  // SINGLE_PUSH
  b2 = SINGLE_PUSH(b1, cb->turn) & ~ALL;
  moves = b2 & checkMask;
  b[size++] = BranchNew(moves, SINGLE_PUSH(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));

  // DOUBLE_PUSH
  moves = SINGLE_PUSH(b2 & ENPASSANT_RANK(cb->turn), cb->turn) & ~ALL & checkMask;
  b[size++] = BranchNew(moves, DOUBLE_PUSH(moves, (!cb->turn)), GET_PIECE(Pawn, cb->turn));

  // Adjust pinned pawns
  // The last four entries correspond to the four pawn moves above:
  //   [size-4]: PAWN_ATTACKS_LEFT
  //   [size-3]: PAWN_ATTACKS_RIGHT
  //   [size-2]: SINGLE_PUSH
  //   [size-1]: DOUBLE_PUSH
  {
    int baseIndex = size - 4;
    b1 = OUR(Pawn) & pinned;
    while (b1)
    {
      s = BitBoardPopLSB(&b1);
      b2 = BitBoardSetBit(EMPTY_BOARD, s);
      b3 = LookupTableGetLineOfSight(l, BitBoardGetLSB(OUR(King)), s);

      // PAWN_ATTACKS_LEFT
      b[baseIndex + 0].to &= ~(PAWN_ATTACKS_LEFT(b2, cb->turn) & ~b3);
      b[baseIndex + 0].from = PAWN_ATTACKS_LEFT(b[baseIndex + 0].to, (!cb->turn));

      // PAWN_ATTACKS_RIGHT
      b[baseIndex + 1].to &= ~(PAWN_ATTACKS_RIGHT(b2, cb->turn) & ~b3);
      b[baseIndex + 1].from = PAWN_ATTACKS_RIGHT(b[baseIndex + 1].to, (!cb->turn));

      // SINGLE_PUSH
      b[baseIndex + 2].to &= ~(SINGLE_PUSH(b2, cb->turn) & ~b3);
      b[baseIndex + 2].from = SINGLE_PUSH(b[baseIndex + 2].to, (!cb->turn));

      // DOUBLE_PUSH
      b[baseIndex + 3].to &= ~(DOUBLE_PUSH(b2, cb->turn) & ~b3);
      b[baseIndex + 3].from = DOUBLE_PUSH(b[baseIndex + 3].to, (!cb->turn));
    }
  }

  // En passant
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
 * If we're at depth 2, we only need to play moves that will interfere
 * with the next move. Otherwise, we can remove these moves from the branch
 * and calculate how many moves have been "saved" by pruning.
 *
 * A move interferes if:
 * -   the to square intersects with any slider piece/pawn move of opponent (including king)
 * -   the to square intersects with any piece of the opponent
 * -   its a double push
 * -   the move somehow disrupts the king moves of next move ie it either attacks the king,
 *     attacks a square the king can move, reliquishes an attack to a square the king can move,
 *     occupies a castling square, or discovered attacks the king - how do we know?
 *       the king moves of the next move are determined by the relevant squares that are
 *       attacked... What if we get the number of times each relevant square is attacked
 *       and then see how this number changes?
 */
int BranchPrune(LookupTable l, ChessBoard *cb, Branch *b, int size)
{
  // Get an array of branches for the next move
  Branch next[BRANCH_SIZE];
  cb->turn = !cb->turn;
  int nextSize = BranchFill(l, cb, next);
  cb->turn = !cb->turn;
  int nextMoves = BranchCount(next, nextSize);

  // Go through all branches of b and start pruning, count how many were pruned
  int movesPruned = 0;
  for (int i = 0; i < size; i++)
  {
    // If a move in the branch
  }

  (void)l;
  (void)cb;
  (void)b;
  (void)size;
  (void)movesPruned;
  (void)nextMoves;
  return 0;
}

// Our pinned pieces and their checking pieces
static BitBoard getCheckingPieces(LookupTable l, ChessBoard *cb, BitBoard them, BitBoard *pinned)
{
  Square ourKing = BitBoardGetLSB(OUR(King));
  BitBoard checking = (PAWN_ATTACKS(OUR(King), cb->turn) & THEIR(Pawn)) |
                      (LookupTableAttacks(l, ourKing, Knight, EMPTY_BOARD) & THEIR(Knight));
  BitBoard candidates = (LookupTableAttacks(l, ourKing, Bishop, them) & (THEIR(Bishop) | THEIR(Queen))) |
                        (LookupTableAttacks(l, ourKing, Rook, them) & (THEIR(Rook) | THEIR(Queen)));

  BitBoard b;
  Square s;
  *pinned = EMPTY_BOARD;
  while (candidates)
  {
    s = BitBoardPopLSB(&candidates);
    b = LookupTableGetSquaresBetween(l, ourKing, s) & ALL & ~them;
    if (b == EMPTY_BOARD)
      checking |= BitBoardSetBit(EMPTY_BOARD, s);
    else if ((b & (b - 1)) == EMPTY_BOARD)
      *pinned |= b;
  }

  return checking;
}

static BitBoard getAttackedSquares(LookupTable l, ChessBoard *cb, BitBoard them)
{
  BitBoard attacked, b;
  BitBoard occupancies = ALL & ~OUR(King);

  attacked = PAWN_ATTACKS(THEIR(Pawn), (!cb->turn));
  b = them & ~THEIR(Pawn);
  while (b)
  {
    Square s = BitBoardPopLSB(&b);
    attacked |= LookupTableAttacks(l, s, GET_TYPE(cb->squares[s]), occupancies);
  }
  return attacked;
}
