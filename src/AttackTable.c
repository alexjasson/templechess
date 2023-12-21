#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "BitBoard.h"
#include "AttackTable.h"

// Maximum size of all possible combinations of relevant occupancies for
// unique pieces (ie. the pieces not including queen)
#define OCCUPANCY_POWERSET_SIZE 4096
// The number of unique pieces
#define PIECE_SIZE 5
#define COLOR_SIZE 2

#define EDGES { 0x0101010101010101, 0x8080808080808080, 0x00000000000000FF, 0xFF00000000000000 }
#define NUM_EDGES 4

typedef enum {
  North, Northeast, East, Southeast, South, Southwest, West, Northwest
} Direction;

struct attackTable {
  BitBoard pieceAttacks[PIECE_SIZE][COLOR_SIZE][BOARD_SIZE][OCCUPANCY_POWERSET_SIZE];

  int occupancySize[PIECE_SIZE][COLOR_SIZE][BOARD_SIZE];
  U64 magicNumbers[PIECE_SIZE][COLOR_SIZE][BOARD_SIZE];
};

static BitBoard getPieceAttacks(Piece p, Color c, Square s, BitBoard occupancies);
static BitBoard getPieceAttack(Piece p, Square s, Direction d, int steps);
static BitBoard getOccupancySet(int index, int occupancySize, BitBoard occupancies);
static BitBoard getRelevantOccupancies(Piece p,  Color c, Square s);

static bool isDiagional(Direction d);

AttackTable AttackTableNew(void) {
  AttackTable a = malloc(sizeof(struct attackTable));
  if (a == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  for (Piece p = Pawn; p <= Rook; p++) {
    for (Color c = White; c <= Black; c++) {
      for (Square s = a1; s <= h8; s++) {
        a->occupancySize[p][c][s] = BitBoardCountBits(getRelevantOccupancies(p, c, s));
      }
    }
  }

  int total = 0;
  for (Piece p = Pawn; p <= Rook; p++) {
    for (Color c = White; c <= Black; c++) {
      for (Square s = a1; s <= h8; s++) {
        if (p == 0) continue;
        printf("Finding magic number for piece: %d, color: %d, square: %d\n", p, c, s);
        a->magicNumbers[p][c][s] = getMagicNumber(p, c, s, a->occupancySize[p][c][s]);
        total++;
        printf("Total: %d\n", total);
      }
    }
  }

  for (Piece p = Pawn; p <= Rook; p++) {
    for (Color c = White; c <= Black; c++) {
      for (Square s = a1; s <= h8; s++) {
        BitBoard relevantOccupancies = getRelevantOccupancies(p, c, s);
        int occupancySize = a->occupancySize[p][c][s];
        int occupancyPowersetSize = 1 << occupancySize;
        for (int i = 0; i < occupancyPowersetSize; i++) {
          BitBoard o = getOccupancySet(i, occupancySize, relevantOccupancies);
          int index = hashFunction(o, a->magicNumbers[p][c][s], occupancySize);
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
  if (p == Queen) {
    return (AttackTableGetPieceAttacks(a, Bishop, c, s, occupancies) |
            AttackTableGetPieceAttacks(a, Rook, c, s, occupancies));
  } else {
    int occupancySize = a->occupancySize[p][c][s];
    BitBoard relevantOccupancies = getRelevantOccupancies(p, c, s);
    int index = hashFunction(relevantOccupancies & occupancies, a->magicNumbers[p][c][s], occupancySize);
    printf("\n\nOccupancy size: %d\n Index: %d\n", occupancySize, index);
    return a->pieceAttacks[p][c][s][index];
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
      else if ((p == King || p == Pawn || p == Knight) && steps > 1) break;
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

static BitBoard getRelevantOccupancies(Piece p,  Color c, Square s) {
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

// All functions above have been tested and work as expected

// just a random number
unsigned int state = 1804289383;

// 32-bit number pseudo random generator
unsigned int generateRandomNumber()
{
	// XOR shift algorithm
	unsigned int x = state;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	state = x;
	return x;
}

// generate random U64 number
U64 randomU64()
{
    // init numbers to randomize
    U64 u1, u2, u3, u4;

    // randomize numbers
    u1 = (U64)(generateRandomNumber()) & 0xFFFF;
    u2 = (U64)(generateRandomNumber()) & 0xFFFF;
    u3 = (U64)(generateRandomNumber()) & 0xFFFF;
    u4 = (U64)(generateRandomNumber()) & 0xFFFF;

    // shuffle bits and return
    return u1 | (u2 << 16) | (u3 << 32) | (u4 << 48);
}

// get random few bits
U64 randomFewbits() {
    return randomU64() & randomU64() & randomU64();
}

int hashFunction(BitBoard key, U64 magicNumber, int occupancySize) {
  return (int)((key * magicNumber) >> (64 - occupancySize));
}

U64 getMagicNumber(Piece p, Color c, Square s, int occupancySize) {
  U64 occupancies[OCCUPANCY_POWERSET_SIZE]; // Change to max
  U64 attacks[OCCUPANCY_POWERSET_SIZE];
  U64 usedAttacks[OCCUPANCY_POWERSET_SIZE];
  int occupancyPowersetSize = 1 << occupancySize;

  BitBoard relevantOccupancies = getRelevantOccupancies(p, c, s);

  for (int i = 0; i < occupancyPowersetSize; i++) {
    occupancies[i] = getOccupancySet(i, occupancySize, relevantOccupancies);
    attacks[i] = getPieceAttacks(p, c, s, occupancies[i]);
  }

  // Test magic numbers
  for (int i = 0; i < 100000000; i++) {
    U64 magicNumber = randomFewbits();

    if(BitBoardCountBits((relevantOccupancies * magicNumber) & 0xFF00000000000000) < 6) continue;

    for (int j = 0; j < occupancyPowersetSize; j++) {
      usedAttacks[j] = 0;
    }

    int collision = 0;
    int index;

    // Test magic index
    for (int j = 0; j < occupancyPowersetSize; j++) {
      index = hashFunction(occupancies[j], magicNumber, occupancySize);
      if (usedAttacks[index] == 0) {
        usedAttacks[index] = attacks[j];
      } else if (usedAttacks[index] != attacks[j]) {
        collision = 1;
      }
    }
    if (!collision) {
      printf("Magic number: %ld maps to index: %d\n", magicNumber, index);
      return magicNumber;
    }
  }
  printf("FAIL");
  return 0;
}

