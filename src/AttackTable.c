#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#include "BitBoard.h"
#include "AttackTable.h"

// Maximum size of all possible combinations of relevant occupancies for
// unique pieces (ie. the pieces not including queen)
#define OCCUPANCY_POWERSET_SIZE 4096
#define PIECE_SIZE 5 // Number of unique pieces
#define COLOR_SIZE 2

#define SEED 1804289383
#define EDGES { 0x0101010101010101, 0x8080808080808080, 0x00000000000000FF, 0xFF00000000000000 }
#define NUM_EDGES 4

typedef enum {
  North, Northeast, East, Southeast, South, Southwest, West, Northwest
} Direction;

struct attackTable {
  // Precalculated attack tables
  BitBoard pieceAttacks[PIECE_SIZE][COLOR_SIZE][BOARD_SIZE][OCCUPANCY_POWERSET_SIZE];

  // Precalculated tables for indexing to the pieceAttacks hash table
  BitBoard relevantOccupancies[PIECE_SIZE][COLOR_SIZE][BOARD_SIZE];
  int occupancySize[PIECE_SIZE][COLOR_SIZE][BOARD_SIZE];
  U64 magicNumbers[PIECE_SIZE][COLOR_SIZE][BOARD_SIZE];
};

static BitBoard getPieceAttacks(Piece p, Color c, Square s, BitBoard occupancies);
static BitBoard getPieceAttack(Piece p, Square s, Direction d, int steps);
static BitBoard getOccupancySet(int index, int occupancySize, BitBoard occupancies);
static BitBoard getRelevantOccupancies(Piece p, Color c, Square s);
static U64 getMagicNumber(Piece p, Color c, Square s, int occupancySize);
static int hash(BitBoard key, U64 magicNumber, int occupancySize);
static bool isDiagional(Direction d);
static U32 Xorshift();
static U64 getRandomNumber();

AttackTable AttackTableNew(void) {
  AttackTable a = malloc(sizeof(struct attackTable));
  if (a == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  for (Piece p = Pawn; p <= Rook; p++) {
    for (Color c = White; c <= Black; c++) {
      for (Square s = a1; s <= h8; s++) {
        a->relevantOccupancies[p][c][s] = getRelevantOccupancies(p, c, s);
        a->occupancySize[p][c][s] = BitBoardCountBits(a->relevantOccupancies[p][c][s]);
        a->magicNumbers[p][c][s] = getMagicNumber(p, c, s, a->occupancySize[p][c][s]);
        printf("Piece: %d, Color: %d, Square: %d, Occupancy Size: %d, Magic Number: %lu\n", p, c, s, a->occupancySize[p][c][s], a->magicNumbers[p][c][s]);
      }
    }
  }

  printf("YO\n");

  for (Piece p = Pawn; p <= Rook; p++) {
    for (Color c = White; c <= Black; c++) {
      for (Square s = a1; s <= h8; s++) {
        int occupancyPowersetSize = 1 << a->occupancySize[p][c][s];
        for (int i = 0; i < occupancyPowersetSize; i++) {
          BitBoard o = getOccupancySet(i, a->occupancySize[p][c][s], a->relevantOccupancies[p][c][s]);
          int index = hash(o, a->magicNumbers[p][c][s], a->occupancySize[p][c][s]);
          a->pieceAttacks[p][c][s][index] = getPieceAttacks(p, c, s, o);
        }
      }
    }
  }

  return a;
}

void AttackTableFree(AttackTable a) {
  free(a);
}

BitBoard AttackTableGetPieceAttacks(AttackTable a, Piece p, Color c, Square s, BitBoard occupancies) {
  if (p != Queen) {
    int index = hash(a->relevantOccupancies[p][c][s] & occupancies, a->magicNumbers[p][c][s], a->occupancySize[p][c][s]);
    return a->pieceAttacks[p][c][s][index];
  } else {
    return (AttackTableGetPieceAttacks(a, Bishop, c, s, occupancies) | AttackTableGetPieceAttacks(a, Rook, c, s, occupancies));
  }
}

// Assume that a queen piece will not be passed to this function
static BitBoard getPieceAttacks(Piece p, Color c, Square s, BitBoard occupancies) {
  BitBoard pieceAttacks = EMPTY_BOARD;

  // Loop through attacks, if attack does not meet criteria for piece/color then break/continue
  for (Direction d = North; d <= Northwest; d++) {
    for (int steps = 1; steps < EDGE_SIZE; steps++) {
      if ((p == Bishop && isDiagional(d)) || (p == Rook && !isDiagional(d))) continue;
      else if (p == Knight && !isDiagional(d)) break;
      else if (p <= King && steps > 1) break;
      else if (p == Pawn && c == White && d != Northeast && d != Northwest) break;
      else if (p == Pawn && c == Black && d != Southeast && d != Southwest) break;

      pieceAttacks |= getPieceAttack(p, s, d, steps);
      if (getPieceAttack(p, s, d, steps) & occupancies) break;
    }
  }
  return pieceAttacks;
}

// Return a bitboard that represents a square that a piece is attacking
static BitBoard getPieceAttack(Piece p, Square s, Direction d, int steps) {
  // Check out of bounds conditions
  int rankOffset = d <= Northeast || d == Northwest ? steps : d >= Southeast && d <= Southwest ? -steps : 0;
  int fileOffset = d >= Northeast && d <= Southeast ? steps : d >= Southwest && d <= Northwest ? -steps : 0;
  int rank = s / EDGE_SIZE;
  int file = s % EDGE_SIZE;
  if ((rank + rankOffset >= EDGE_SIZE || rank + rankOffset < 0) ||
      (file + fileOffset >= EDGE_SIZE || file + fileOffset < 0)) {
    return EMPTY_BOARD;
  }

  if (!(p == Knight && steps == 1)) {
    return BitBoardSetBit(EMPTY_BOARD, s + EDGE_SIZE * rankOffset + fileOffset);
  }

  // Handle case where knight hasn't finished it's move
  int offset = (d == North) ? EDGE_SIZE : (d == South) ? -EDGE_SIZE : (d == East) ? 1 : -1;
  Direction d1 = (d == North || d == South) ? East : North;
  Direction d2 = (d == North || d == South) ? West : South;
  return (getPieceAttack(p, s + offset, d1, 2) | getPieceAttack(p, s + offset, d2, 2));
}

static bool isDiagional(Direction d) {
  return d % 2 == 0;
}

// get unique set of occupancies for index
static BitBoard getOccupancySet(int index, int occupancySize, BitBoard occupancies) {
  BitBoard occupancy = EMPTY_BOARD;
  for (int i = 0; i < occupancySize; i++) {
    int square = BitBoardLeastSignificantBit(occupancies);
    occupancies = BitBoardPopBit(occupancies, square);
    if (index & (1 << i)) occupancy = BitBoardSetBit(occupancy, square);
  }
  return occupancy;
}

static BitBoard getRelevantOccupancies(Piece p, Color c, Square s) {
  BitBoard relevantOccupancies = getPieceAttacks(p, c, s, EMPTY_BOARD);
  BitBoard edges[NUM_EDGES] = EDGES;
  BitBoard piece = BitBoardSetBit(EMPTY_BOARD, s);
  for (int i = 0; i < NUM_EDGES; i++) {
    if (!((piece & edges[i]) == piece)) {
      relevantOccupancies &= ~edges[i];
    }
  }
  return relevantOccupancies;
}

// 32-bit number pseudo random generator - not thread safe
static U32 Xorshift() {
  static U32 state = 0;
  static bool seeded = false;

  // Seed only once
  if (!seeded) {
    state = SEED;
    seeded = true;
  }

  U32 x = state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;

  return state = x;
}

// generate random U64 number
static U64 getRandomNumber() {
  U64 u1, u2, u3, u4;

  u1 = (U64)(Xorshift()) & 0xFFFF;
  u2 = (U64)(Xorshift()) & 0xFFFF;
  u3 = (U64)(Xorshift()) & 0xFFFF;
  u4 = (U64)(Xorshift()) & 0xFFFF;

  return u1 | (u2 << 16) | (u3 << 32) | (u4 << 48);
}

static int hash(BitBoard key, U64 magicNumber, int occupancySize) {
  return (int)((key * magicNumber) >> (64 - occupancySize));
}

static U64 getMagicNumber(Piece p, Color c, Square s, int occupancySize) {
  BitBoard occupancies[OCCUPANCY_POWERSET_SIZE];
  BitBoard attacks[OCCUPANCY_POWERSET_SIZE];
  BitBoard usedAttacks[OCCUPANCY_POWERSET_SIZE];

  BitBoard relevantOccupancies = getRelevantOccupancies(p, c, s);
  int occupancyPowersetSize = 1 << occupancySize;

  for (int i = 0; i < occupancyPowersetSize; i++) {
    occupancies[i] = getOccupancySet(i, occupancySize, relevantOccupancies);
    attacks[i] = getPieceAttacks(p, c, s, occupancies[i]);
  }

  for (int i = 0; i < INT_MAX; i++) {
    U64 magicNumber = getRandomNumber() & getRandomNumber() & getRandomNumber();

    for (int j = 0; j < occupancyPowersetSize; j++) usedAttacks[j] = EMPTY_BOARD;
    int collision = false;

    // Test magic index
    for (int j = 0; j < occupancyPowersetSize; j++) {
      int index = hash(occupancies[j], magicNumber, occupancySize);
      if (usedAttacks[index] == 0) {
        usedAttacks[index] = attacks[j];
      } else if (usedAttacks[index] != attacks[j]) {
        collision = true;
      }
    }
    if (!collision) return magicNumber;
  }
  fprintf(stderr, "Failed to find magic number!\n");
  exit(1);
}

