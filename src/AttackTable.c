#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

#include "BitBoard.h"
#include "AttackTable.h"

// Maximum size of all possible combinations of relevant occupancies for
// unique pieces (ie. the pieces not including queen)
#define OCCUPANCY_POWERSET_SIZE 4096
// The number of unique pieces
#define PIECE_SIZE 5
#define COLOR_SIZE 2

#define EDGES { 0x0101010101010101, 0x8080808080808080, 0x00000000000000FF, 0xFF00000000000000 }

typedef enum {
  North, Northeast, East, Southeast, South, Southwest, West, Northwest
} Direction;

struct attackTable {
  BitBoard pieceAttacks[PIECE_SIZE][COLOR_SIZE][BOARD_SIZE][OCCUPANCY_POWERSET_SIZE];
};

static BitBoard getPieceAttacks(Piece p, Color c, Square s, BitBoard occupancies);
static BitBoard getPieceAttack(Piece p, Square s, Direction d, int steps);
static BitBoard getOccupancySet(int index, int numBits, BitBoard occupancies);
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
        BitBoard relevantOccupancies = getRelevantOccupancies(p, c, s);
        int numBits = BitBoardCountBits(relevantOccupancies);
        int occupancyPowersetSize = 1 << numBits;
        for (int i = 0; i < occupancyPowersetSize; i++) {
          printf("Piece: %d, Color: %d, Square: %d, NumBits: %d PowersetSize: %d\n", p, c, s, numBits, occupancyPowersetSize);
          printf("Occupancy set bitboard:\n");
          BitBoardPrint(getOccupancySet(i, numBits, relevantOccupancies));
          printf("Piece attacks bitboard:\n");
          BitBoardPrint(getPieceAttacks(p, c, s, getOccupancySet(i, numBits, relevantOccupancies)));
          printf("------------------\n\n");
          usleep(10000);
          a->pieceAttacks[p][c][s][i] = getOccupancySet(i, numBits, relevantOccupancies);
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
    return a->pieceAttacks[p][c][s][occupancies];
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

// Generate unique set of occupancies
static BitBoard getOccupancySet(int index, int numBits, BitBoard occupancies) {
  BitBoard occupancy = EMPTY_BOARD;
  for (int i = 0; i < numBits; i++) {
    int square = BitBoardLeastSignificantBit(occupancies);
    occupancies = BitBoardPopBit(occupancies, square);
    if (index & (1 << i)) occupancy = BitBoardSetBit(occupancy, square);
  }
  return occupancy;
}

static BitBoard getRelevantOccupancies(Piece p,  Color c, Square s) {
  BitBoard relevantOccupancies = getPieceAttacks(p, c, s, EMPTY_BOARD);
  BitBoard edges[4] = EDGES;
  BitBoard piece = BitBoardSetBit(EMPTY_BOARD, s);
  for (int i = 0; i < 4; i++) {
    if (!((piece & edges[i]) == piece)) {
      relevantOccupancies &= ~edges[i];
    }
  }
  return relevantOccupancies;
}