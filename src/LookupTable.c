#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "BitBoard.h"
#include "LookupTable.h"
#include "utility.h"

#define IS_DIAGONAL(d) (d % 2 == 1)
#define GET_POWERSET_SIZE(n) (1 << n)

#define HASH_FILEPATH "data/hash.dat"

#define PAWN_MOVES_POWERSET 2
#define CASTLING_POWERSET 256
#define BISHOP_ATTACKS_POWERSET 512
#define ROOK_ATTACKS_POWERSET 4096

#define CASTLING_RIGHTS_MASK 0x9100000000000091
#define CASTLING_ATTACK_MASK 0x6C0000000000006C
#define CASTLING_OCCUPANCY_MASK 0x6E0000000000006E

#define DEFAULT_COLOR White

typedef enum {
  North, Northeast, East, Southeast, South, Southwest, West, Northwest
} Direction;

typedef struct {
  BitBoard bits;
  int bitShift;
  uint64_t magicNumber;
} HashData;

struct lookupTable {
  BitBoard knightAttacks[BOARD_SIZE];
  BitBoard kingAttacks[BOARD_SIZE];
  BitBoard bishopAttacks[BOARD_SIZE][BISHOP_ATTACKS_POWERSET];
  BitBoard rookAttacks[BOARD_SIZE][ROOK_ATTACKS_POWERSET];
  HashData bishopData[BOARD_SIZE]; // Used for bishop attacks
  HashData rookData[BOARD_SIZE]; // Used for rook attacks

  BitBoard squaresBetween[BOARD_SIZE][BOARD_SIZE]; // Squares Between exclusive
  BitBoard lineOfSight[BOARD_SIZE][BOARD_SIZE]; // All squares of a rank/file/diagonal/antidiagonal
  BitBoard rank[BOARD_SIZE];
};

static BitBoard getMove(Square s, Type t, Direction d, int steps);
static BitBoard getAttacks(Square s, Type t, Color c, BitBoard occupancies);

static BitBoard getRelevantBits(Square s, Type t, Color c);
static BitBoard getBitsSubset(int index, BitBoard bits);
static HashData getHashData(Square s, Type t, Color c);
static int magicHash(HashData h, BitBoard occupancies);

static BitBoard getSquaresBetween(LookupTable l, Square s1, Square s2);
static BitBoard getLineOfSight(LookupTable l, Square s1, Square s2);
static BitBoard getRank(Square s);

static void initializeMoveTables(LookupTable l);
static void initializeHashData(LookupTable l);
static void initializeHelperTables(LookupTable l);

LookupTable LookupTableNew(void) {
  LookupTable l = malloc(sizeof(struct lookupTable));
  if (l == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  initializeHashData(l);
  initializeMoveTables(l);
  initializeHelperTables(l);

  return l;
}

void initializeMoveTables(LookupTable l) {
  // Minor/major piece moves
  for (Square s = a8; s <= h1; s++) {
    l->knightAttacks[s] = getAttacks(s, Knight, DEFAULT_COLOR, EMPTY_BOARD);
    l->kingAttacks[s] = getAttacks(s, King, DEFAULT_COLOR, EMPTY_BOARD);
    for (int i = 0; i < BISHOP_ATTACKS_POWERSET; i++) {
      BitBoard occupancies = getBitsSubset(i, l->bishopData[s].bits);
      int index = magicHash(l->bishopData[s], occupancies);
      l->bishopAttacks[s][index] = getAttacks(s, Bishop, DEFAULT_COLOR, occupancies);
    }
    for (int i = 0; i < ROOK_ATTACKS_POWERSET; i++) {
      BitBoard occupancies = getBitsSubset(i, l->rookData[s].bits);
      int index = magicHash(l->rookData[s], occupancies);
      l->rookAttacks[s][index] = getAttacks(s, Rook, DEFAULT_COLOR, occupancies);
    }
  }
}

void initializeHashData(LookupTable l) {
  if (!isFileEmpty(HASH_FILEPATH)) {
    readFromFile(l->bishopData, sizeof(HashData), BOARD_SIZE, HASH_FILEPATH, 0);
    readFromFile(l->rookData, sizeof(HashData), BOARD_SIZE, HASH_FILEPATH, sizeof(HashData) * BOARD_SIZE);
  } else {
    for (Square s = a8; s <= h1; s++) {
      l->bishopData[s] = getHashData(s, Bishop, DEFAULT_COLOR);
      l->rookData[s] = getHashData(s, Rook, DEFAULT_COLOR);
    }
    writeToFile(l->bishopData, sizeof(HashData), BOARD_SIZE, HASH_FILEPATH, 0);
    writeToFile(l->rookData, sizeof(HashData), BOARD_SIZE, HASH_FILEPATH, sizeof(HashData) * BOARD_SIZE);
  }
}

void initializeHelperTables(LookupTable l) {
  for (Square s1 = a8; s1 <= h1; s1++) {
    for (Square s2 = a8; s2 <= h1; s2++) {
      l->squaresBetween[s1][s2] = getSquaresBetween(l, s1, s2);
      l->lineOfSight[s1][s2] = getLineOfSight(l, s1, s2);
    }
    l->rank[s1] = getRank(s1);
  }
}

void LookupTableFree(LookupTable l) {
  free(l);
}

BitBoard LookupTableKnightAttacks(LookupTable l, Square s) {
  return l->knightAttacks[s];
}

BitBoard LookupTableKingAttacks(LookupTable l, Square s) {
  return l->kingAttacks[s];
}

BitBoard LookupTableBishopAttacks(LookupTable l, Square s, BitBoard occupancies) {
  int index = magicHash(l->bishopData[s], occupancies);
  return l->bishopAttacks[s][index];
}

BitBoard LookupTableRookAttacks(LookupTable l, Square s, BitBoard occupancies) {
  int index = magicHash(l->rookData[s], occupancies);
  return l->rookAttacks[s][index];
}

BitBoard LookupTableGetSquaresBetween(LookupTable l, Square s1, Square s2) {
  return l->squaresBetween[s1][s2];
}

BitBoard LookupTableGetLineOfSight(LookupTable l, Square s1, Square s2) {
  return l->lineOfSight[s1][s2];
}

BitBoard LookupTableGetRank(LookupTable l, Square s) {
  return l->rank[s];
}

static BitBoard getAttacks(Square s, Type t, Color c, BitBoard occupancies) {
  BitBoard attacks = EMPTY_BOARD;

  // Loop through attacks, if attack does not meet criteria for piece/color then break/continue
  for (Direction d = North; d <= Northwest; d++) {
    for (int steps = 1; steps < EDGE_SIZE; steps++) {
      if ((t == Bishop && !IS_DIAGONAL(d)) || (t == Rook && IS_DIAGONAL(d))) continue;
      else if (t == Knight && IS_DIAGONAL(d)) break;
      else if (t <= Knight && steps > 1) break;
      else if (t == Pawn && c == White && d != Northeast && d != Northwest) break;
      else if (t == Pawn && c == Black && d != Southeast && d != Southwest) break;

      BitBoard attack = getMove(s, t, d, steps);
      bool capture = attack & occupancies;
      attacks |= attack;
      if (capture) break;
    }
  }
  return attacks;
}

// Assume knight or queen will not be passed to this function
static BitBoard getRelevantBits(Square s, Type t, Color c) {
  BitBoard relevantBits = EMPTY_BOARD;

  if (t == Pawn) {
    Direction d = (c == White) ? North : South;
    relevantBits = getMove(s, Pawn, d, 1);
  } else if (t == King) {
    relevantBits = (c == White) ? SOUTH_EDGE : NORTH_EDGE;
  } else {
    relevantBits = getAttacks(s, t, c, EMPTY_BOARD);
    BitBoard piece = BitBoardSetBit(EMPTY_BOARD, s);
    if (piece & ~NORTH_EDGE) relevantBits &= ~NORTH_EDGE;
    if (piece & ~SOUTH_EDGE) relevantBits &= ~SOUTH_EDGE;
    if (piece & ~EAST_EDGE) relevantBits &= ~EAST_EDGE;
    if (piece & ~WEST_EDGE) relevantBits &= ~WEST_EDGE;
  }

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

static int magicHash(HashData h, BitBoard occupancies) {
  return (int)(((h.bits & occupancies) * h.magicNumber) >> (h.bitShift));
}

static HashData getHashData(Square s, Type t, Color c) {
  HashData h;
  h.bits = getRelevantBits(s, t, c);

  // Return early if piece is pawn or king since they do not use magicHash
  if (t == Pawn) {
    if (c == White) h.bitShift = (s - EDGE_SIZE) % BOARD_SIZE;
    else h.bitShift = (s + EDGE_SIZE) % BOARD_SIZE;
    return h;
  } else if (t == King) {
    if (c == White) h.bitShift = BOARD_SIZE - EDGE_SIZE;
    else h.bitShift = 0;
    return h;
  }

  h.bitShift = BOARD_SIZE - BitBoardCountBits(h.bits);
  int relevantBitsPowersetSize = GET_POWERSET_SIZE((BOARD_SIZE - h.bitShift));

  BitBoard *relevantBitsPowerset = malloc(sizeof(BitBoard) * relevantBitsPowersetSize);
  BitBoard *attacks = malloc(sizeof(BitBoard) * relevantBitsPowersetSize);
  BitBoard *usedAttacks = malloc(sizeof(BitBoard) * relevantBitsPowersetSize);
  if (relevantBitsPowerset == NULL || attacks == NULL || usedAttacks == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < relevantBitsPowersetSize; i++) {
    relevantBitsPowerset[i] = getBitsSubset(i, h.bits);
    attacks[i] = getAttacks(s, t, c, relevantBitsPowerset[i]);
  }

  while (true) {
    uint64_t magicNumberCandidate = getRandomNumber() & getRandomNumber() & getRandomNumber();

    for (int j = 0; j < relevantBitsPowersetSize; j++) usedAttacks[j] = EMPTY_BOARD;
    int collision = false;

    // Test magic index
    for (int j = 0; j < relevantBitsPowersetSize; j++) {
      h.magicNumber = magicNumberCandidate;
      int index = magicHash(h, relevantBitsPowerset[j]);
      if (usedAttacks[index] == EMPTY_BOARD) {
        usedAttacks[index] = attacks[j];
      } else if (usedAttacks[index] != attacks[j]) {
        collision = true;
      }
    }
    if (!collision) break;
  }

  free(relevantBitsPowerset);
  free(attacks);
  free(usedAttacks);
  return h;
}

static BitBoard getSquaresBetween(LookupTable l, Square s1, Square s2) {
  BitBoard pieces = BitBoardSetBit(EMPTY_BOARD, s1) | BitBoardSetBit(EMPTY_BOARD, s2);
  BitBoard squaresBetween = EMPTY_BOARD;
  if (BitBoardGetFile(s1) == BitBoardGetFile(s2) || BitBoardGetRank(s1) == BitBoardGetRank(s2)) {
    squaresBetween = LookupTableRookAttacks(l, s1, pieces) &
                     LookupTableRookAttacks(l, s2, pieces);
  } else if (BitBoardGetDiagonal(s1) == BitBoardGetDiagonal(s2) ||
             BitBoardGetAntiDiagonal(s1) == BitBoardGetAntiDiagonal(s2)) {
    squaresBetween = LookupTableBishopAttacks(l, s1, pieces) &
                     LookupTableBishopAttacks(l, s2, pieces);
  }
  return squaresBetween;
}

static BitBoard getLineOfSight(LookupTable l, Square s1, Square s2) {
  BitBoard lineOfSight = EMPTY_BOARD;
  if (BitBoardGetFile(s1) == BitBoardGetFile(s2) || BitBoardGetRank(s1) == BitBoardGetRank(s2)) {
    lineOfSight = (LookupTableRookAttacks(l, s1, EMPTY_BOARD) &
                   LookupTableRookAttacks(l, s2, EMPTY_BOARD)) |
                   BitBoardSetBit(EMPTY_BOARD, s2);
  } else if (BitBoardGetDiagonal(s1) == BitBoardGetDiagonal(s2) ||
             BitBoardGetAntiDiagonal(s1) == BitBoardGetAntiDiagonal(s2)) {
    lineOfSight = (LookupTableBishopAttacks(l, s1, EMPTY_BOARD) &
                   LookupTableBishopAttacks(l, s2, EMPTY_BOARD)) |
                   BitBoardSetBit(EMPTY_BOARD, s2);
  }
  return lineOfSight;
}

// Returns the rank of a square
static BitBoard getRank(Square s) {
  return SOUTH_EDGE >> (EDGE_SIZE * (EDGE_SIZE - BitBoardGetRank(s) - 1));
}