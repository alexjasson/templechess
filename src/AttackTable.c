#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#include "BitBoard.h"
#include "AttackTable.h"
#include "utility.h"

#define PIECE_SIZE 6
#define COLOR_SIZE 2

#define EDGES { 0x0101010101010101, 0x8080808080808080, 0x00000000000000FF, 0xFF00000000000000 }
#define NUM_EDGES 4

#define MAGIC_NUMBERS_FILEPATH "data/magicNumbers.dat"

typedef enum {
  North, Northeast, East, Southeast, South, Southwest, West, Northwest
} Direction;

struct attackTable {
  // Precalculated attack tables
  BitBoard *pieceAttacks[PIECE_SIZE - 1][COLOR_SIZE][BOARD_SIZE];

  // Precalculated tables for indexing to the pieceAttacks hash table
  BitBoard relevantOccupancies[PIECE_SIZE - 1][COLOR_SIZE][BOARD_SIZE];
  int occupancySize[PIECE_SIZE - 1][COLOR_SIZE][BOARD_SIZE];
  U64 magicNumbers[PIECE_SIZE - 1][COLOR_SIZE][BOARD_SIZE];
};

static BitBoard getPieceAttacks(Piece p, Color c, Square s, BitBoard occupancies);
static BitBoard getPieceAttack(Piece p, Square s, Direction d, int steps);
static BitBoard getOccupanciesSubset(int index, int occupancySize, BitBoard occupancies);
static BitBoard getRelevantOccupancies(Piece p, Color c, Square s);
static U64 getMagicNumber(Piece p, Color c, Square s, int occupancySize);
static int hash(BitBoard key, U64 magicNumber, int occupancySize);
static bool isDiagional(Direction d);
static BitBoard *getAllPieceAttacks(Piece p, Color c, Square s, AttackTable a);
static int getPowersetSize(int setSize);

AttackTable AttackTableNew(void) {
  AttackTable a = malloc(sizeof(struct attackTable));
  if (a == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  bool emptyFile = isFileEmpty(MAGIC_NUMBERS_FILEPATH);
  int fileElementsSize = (PIECE_SIZE - 1) * COLOR_SIZE * BOARD_SIZE;

  if (!emptyFile) readFromFile(a->magicNumbers, sizeof(U64), fileElementsSize, MAGIC_NUMBERS_FILEPATH);

  for (Piece p = Pawn; p <= Rook; p++) {
    for (Color c = White; c <= Black; c++) {
      for (Square s = a1; s <= h8; s++) {
        a->relevantOccupancies[p][c][s] = getRelevantOccupancies(p, c, s);
        a->occupancySize[p][c][s] = BitBoardCountBits(a->relevantOccupancies[p][c][s]);
        if (emptyFile) a->magicNumbers[p][c][s] = getMagicNumber(p, c, s, a->occupancySize[p][c][s]);
        a->pieceAttacks[p][c][s] = getAllPieceAttacks(p, c, s, a);
      }
    }
  }

  if (emptyFile) writeToFile(a->magicNumbers, sizeof(U64), fileElementsSize, MAGIC_NUMBERS_FILEPATH);

  return a;
}

void AttackTableFree(AttackTable a) {
  for (Piece p = Pawn; p <= Rook; p++) {
    for (Color c = White; c <= Black; c++) {
      for (Square s = a1; s <= h8; s++) {
        free(a->pieceAttacks[p][c][s]);
      }
    }
  }
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

static BitBoard *getAllPieceAttacks(Piece p, Color c, Square s, AttackTable a) {
  int occupancyPowersetSize = getPowersetSize(a->occupancySize[p][c][s]);
  BitBoard *allPieceAttacks = malloc(sizeof(BitBoard) * occupancyPowersetSize);
  if (allPieceAttacks == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < occupancyPowersetSize; i++) {
    BitBoard o = getOccupanciesSubset(i, a->occupancySize[p][c][s], a->relevantOccupancies[p][c][s]);
    int index = hash(o, a->magicNumbers[p][c][s], a->occupancySize[p][c][s]);
    allPieceAttacks[index] = getPieceAttacks(p, c, s, o);
  }
  return allPieceAttacks;
}

static int getPowersetSize(int setSize) {
  return 1 << setSize;
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
static BitBoard getOccupanciesSubset(int index, int occupancySize, BitBoard occupancies) {
  BitBoard occupanciesSubset = EMPTY_BOARD;
  for (int i = 0; i < occupancySize; i++) {
    int square = BitBoardLeastSignificantBit(occupancies);
    occupancies = BitBoardPopBit(occupancies, square);
    if (index & (1 << i)) occupanciesSubset = BitBoardSetBit(occupanciesSubset, square);
  }
  return occupanciesSubset;
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

static int hash(BitBoard key, U64 magicNumber, int occupancySize) {
  return (int)((key * magicNumber) >> (64 - occupancySize));
}

static U64 getMagicNumber(Piece p, Color c, Square s, int occupancySize) {
  int occupancyPowersetSize = getPowersetSize(occupancySize);
  BitBoard occupancies[occupancyPowersetSize];
  BitBoard attacks[occupancyPowersetSize];
  BitBoard usedAttacks[occupancyPowersetSize];
  BitBoard relevantOccupancies = getRelevantOccupancies(p, c, s);

  for (int i = 0; i < occupancyPowersetSize; i++) {
    occupancies[i] = getOccupanciesSubset(i, occupancySize, relevantOccupancies);
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

