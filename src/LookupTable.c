#include <stdio.h>
#include <stdlib.h>
#include <immintrin.h>

#include "BitBoard.h"
#include "LookupTable.h"

#define IS_DIAGONAL(d) (d % 2 == 1)
#define BISHOP_ATTACKS_POWERSET 512
#define ROOK_ATTACKS_POWERSET 4096

typedef enum {
  North, Northeast, East, Southeast, South, Southwest, West, Northwest
} Direction;

struct lookupTable {
  BitBoard knightAttacks[BOARD_SIZE];
  BitBoard kingAttacks[BOARD_SIZE];
  BitBoard bishopAttacks[BOARD_SIZE][BISHOP_ATTACKS_POWERSET];
  BitBoard rookAttacks[BOARD_SIZE][ROOK_ATTACKS_POWERSET];
  BitBoard bishopBits[BOARD_SIZE];
  BitBoard rookBits[BOARD_SIZE];

  BitBoard squaresBetween[BOARD_SIZE][BOARD_SIZE]; // Squares Between exclusive
  BitBoard lineOfSight[BOARD_SIZE][BOARD_SIZE]; // All squares of a rank/file/diagonal/antidiagonal
};

static BitBoard getMove(Square s, Type t, Direction d, int steps);
static BitBoard getAttacks(Square s, Type t, BitBoard occupancies);
static BitBoard getRelevantBits(Square s, Type t);
static BitBoard getBitsSubset(int index, BitBoard bits);
static int hash(BitBoard occupancies, BitBoard relevantBits);

static BitBoard getSquaresBetween(LookupTable l, Square s1, Square s2);
static BitBoard getLineOfSight(LookupTable l, Square s1, Square s2);

static void initializeLookupTable(LookupTable l);

LookupTable LookupTableNew(void) {
  LookupTable l = malloc(sizeof(struct lookupTable));
  if (l == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  initializeLookupTable(l);

  return l;
}

void initializeLookupTable(LookupTable l) {
  for (Square s1 = a8; s1 <= h1; s1++) {
    l->knightAttacks[s1] = getAttacks(s1, Knight, EMPTY_BOARD);
    l->kingAttacks[s1] = getAttacks(s1, King, EMPTY_BOARD);
    l->bishopBits[s1] = getRelevantBits(s1, Bishop);
    l->rookBits[s1] = getRelevantBits(s1, Rook);
    for (int i = 0; i < BISHOP_ATTACKS_POWERSET; i++) {
      BitBoard occupancies = getBitsSubset(i, l->bishopBits[s1]);
      int index = hash(occupancies, l->bishopBits[s1]);
      l->bishopAttacks[s1][index] = getAttacks(s1, Bishop, occupancies);
    }
    for (int i = 0; i < ROOK_ATTACKS_POWERSET; i++) {
      BitBoard occupancies = getBitsSubset(i, l->rookBits[s1]);
      int index = hash(occupancies, l->rookBits[s1]);
      l->rookAttacks[s1][index] = getAttacks(s1, Rook, occupancies);
    }
  }

  for (Square s1 = a8; s1 <= h1; s1++) {
    for (Square s2 = a8; s2 <= h1; s2++) {
      l->squaresBetween[s1][s2] = getSquaresBetween(l, s1, s2);
      l->lineOfSight[s1][s2] = getLineOfSight(l, s1, s2);
    }
  }
}

// We use the pext instruction as a perfect hash function
int hash(BitBoard occupancies, BitBoard relevantBits) {
  return _pext_u64(occupancies, relevantBits);
}

void LookupTableFree(LookupTable l) {
  free(l);
}

BitBoard LookupTableAttacks(LookupTable l, Square s, Type t, BitBoard occupancies) {
  switch (t) {
    case Knight: return l->knightAttacks[s];
    case King: return l->kingAttacks[s];
    case Bishop: return l->bishopAttacks[s][hash(occupancies, l->bishopBits[s])];
    case Rook: return l->rookAttacks[s][hash(occupancies, l->rookBits[s])];
    case Queen: return l->bishopAttacks[s][hash(occupancies, l->bishopBits[s])] |
                       l->rookAttacks[s][hash(occupancies, l->rookBits[s])];
    default: printf("Piece: %d\n", t); // "Invalid piece type\n
             fprintf(stderr, "Invalid piece type\n"); exit(EXIT_FAILURE);
  }
}

BitBoard LookupTableGetSquaresBetween(LookupTable l, Square s1, Square s2) {
  return l->squaresBetween[s1][s2];
}

BitBoard LookupTableGetLineOfSight(LookupTable l, Square s1, Square s2) {
  return l->lineOfSight[s1][s2];
}

static BitBoard getAttacks(Square s, Type t, BitBoard occupancies) {
  BitBoard attacks = EMPTY_BOARD;

  // Loop through attacks, if attack does not meet criteria for piece/color then break/continue
  for (Direction d = North; d <= Northwest; d++) {
    for (int steps = 1; steps < EDGE_SIZE; steps++) {
      if ((t == Bishop && !IS_DIAGONAL(d)) || (t == Rook && IS_DIAGONAL(d))) continue;
      else if (t == Knight && IS_DIAGONAL(d)) break;
      else if (t <= Knight && steps > 1) break;

      BitBoard attack = getMove(s, t, d, steps);
      attacks |= attack;
      if (attack & occupancies) break;
    }
  }
  return attacks;
}

// Assume knight or queen will not be passed to this function
static BitBoard getRelevantBits(Square s, Type t) {
  BitBoard relevantBits = EMPTY_BOARD;
  relevantBits = getAttacks(s, t, EMPTY_BOARD);
  BitBoard piece = BitBoardSetBit(EMPTY_BOARD, s);
  if (piece & ~NORTH_EDGE) relevantBits &= ~NORTH_EDGE;
  if (piece & ~SOUTH_EDGE) relevantBits &= ~SOUTH_EDGE;
  if (piece & ~EAST_EDGE) relevantBits &= ~EAST_EDGE;
  if (piece & ~WEST_EDGE) relevantBits &= ~WEST_EDGE;
  return relevantBits;
}

// Return a bitboard that represents a square that a piece is attacking
static BitBoard getMove(Square s, Type t, Direction d, int steps) {
    int rankOffset = (d >= Southeast && d <= Southwest) ? steps : (d <= Northeast || d == Northwest) ? -steps : 0;
    int fileOffset = (d >= Northeast && d <= Southeast) ? steps : (d >= Southwest && d <= Northwest) ? -steps : 0;
    int rank = BitBoardGetRank(s);
    int file = BitBoardGetFile(s);

    // Check for out-of-bounds conditions
    if ((rank + rankOffset >= EDGE_SIZE || rank + rankOffset < 0) ||
        (file + fileOffset >= EDGE_SIZE || file + fileOffset < 0)) {
        return EMPTY_BOARD;
    }

    if (!(t == Knight && steps == 1)) {
        return BitBoardSetBit(EMPTY_BOARD, s + EDGE_SIZE * rankOffset + fileOffset);
    }

    // Handle case where the knight hasn't finished its move
    int offset = (d == North) ? -EDGE_SIZE : (d == South) ? EDGE_SIZE : (d == East) ? 1 : -1;
    Direction d1 = (d == North || d == South) ? East : North;
    Direction d2 = (d == North || d == South) ? West : South;
    return (getMove(s + offset, t, d1, 2) | getMove(s + offset, t, d2, 2));
}

static BitBoard getBitsSubset(int index, BitBoard bits) {
  int numBits = BitBoardCountBits(bits);
  BitBoard relevantBitsSubset = EMPTY_BOARD;
  for (int i = 0; i < numBits; i++) {
    Square s = BitBoardGetLSB(bits);
    bits = BitBoardPopBit(bits, s);
    if (index & (1 << i)) relevantBitsSubset = BitBoardSetBit(relevantBitsSubset, s);
  }
  return relevantBitsSubset;
}

static BitBoard getSquaresBetween(LookupTable l, Square s1, Square s2) {
  BitBoard pieces = BitBoardSetBit(EMPTY_BOARD, s1) | BitBoardSetBit(EMPTY_BOARD, s2);
  BitBoard squaresBetween = EMPTY_BOARD;
  if (BitBoardGetFile(s1) == BitBoardGetFile(s2) || BitBoardGetRank(s1) == BitBoardGetRank(s2)) {
    squaresBetween = LookupTableAttacks(l, s1, Rook, pieces) &
                     LookupTableAttacks(l, s2, Rook, pieces);
  } else if (BitBoardGetDiagonal(s1) == BitBoardGetDiagonal(s2) ||
             BitBoardGetAntiDiagonal(s1) == BitBoardGetAntiDiagonal(s2)) {
    squaresBetween = LookupTableAttacks(l, s1, Bishop, pieces) &
                     LookupTableAttacks(l, s2, Bishop, pieces);
  }
  return squaresBetween;
}

static BitBoard getLineOfSight(LookupTable l, Square s1, Square s2) {
  BitBoard lineOfSight = EMPTY_BOARD;
  if (BitBoardGetFile(s1) == BitBoardGetFile(s2) || BitBoardGetRank(s1) == BitBoardGetRank(s2)) {
    lineOfSight = (LookupTableAttacks(l, s1, Rook, EMPTY_BOARD) &
                   LookupTableAttacks(l, s2, Rook, EMPTY_BOARD)) |
                   BitBoardSetBit(EMPTY_BOARD, s2);
  } else if (BitBoardGetDiagonal(s1) == BitBoardGetDiagonal(s2) ||
             BitBoardGetAntiDiagonal(s1) == BitBoardGetAntiDiagonal(s2)) {
    lineOfSight = (LookupTableAttacks(l, s1, Bishop, EMPTY_BOARD) &
                   LookupTableAttacks(l, s2, Bishop, EMPTY_BOARD)) |
                   BitBoardSetBit(EMPTY_BOARD, s2);
  }
  return lineOfSight;
}