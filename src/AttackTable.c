#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "BitBoard.h"
#include "AttackTable.h"

// Maximum size of all possible combinations of relevant occupancies for any piece
#define OCCUPANCY_POWERSET_SIZE 4096
#define PIECE_SIZE 6
#define COLOR_SIZE 2

typedef enum {
  North, Northeast, East, Southeast, South, Southwest, West, Northwest
} Direction;

struct attackTable {
  BitBoard pieceAttacks[5][COLOR_SIZE][BOARD_SIZE];
};

static BitBoard getPieceAttacks(Piece p, Color c, Square s, BitBoard occupancies);
static BitBoard getPieceAttack(Piece p, Square s, Direction d, int steps);
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
        // loop through occupancies powerset
        a->pieceAttacks[p][c][s] = getPieceAttacks(p, c, s, EMPTY_BOARD);
      }
    }
  }

  return a;
}

void AttackTableFree(AttackTable a) {
  free(a);
}

BitBoard AttackTableGetPieceAttacks(AttackTable a, Piece p, Color c, Square s) {
  return a->pieceAttacks[p][c][s];
}

// Assume that a queen piece will not be passed to this function
static BitBoard getPieceAttacks(Piece p, Color c, Square s, BitBoard occupancies) {
  BitBoard pieceAttacks = EMPTY_BOARD;

  // Loop through moves, if move does not meet criteria for piece/color then break/continue
  for (Direction d = North; d <= Northwest; d++) {
    for (int steps = 1; steps < EDGE_SIZE; steps++) {
      if ((p == Bishop && isDiagional(d)) || (p == Rook && !isDiagional(d))) continue;
      if (p == Knight && !isDiagional(d)) break;
      if ((p == King || p == Pawn || p == Knight) && steps > 1) break;
      if (p == Pawn && c == White && d != Northeast && d != Northwest) break;
      if (p == Pawn && c == Black && d != Southeast && d != Southwest) break;

      pieceAttacks |= getPieceAttack(p, s, d, steps);
      if (getPieceAttack(p, s, d, steps) & occupancies) break;
    }
  }
  return pieceAttacks;
}

static BitBoard getPieceAttack(Piece p, Square s, Direction d, int steps) {
  if (steps >= EDGE_SIZE) return EMPTY_BOARD;

  // Check attack will not be horizontally out of bounds
  int file = s % EDGE_SIZE;
  if (d >= Northeast && d <= Southeast && (file + steps >= EDGE_SIZE)) return EMPTY_BOARD;
  if (d >= Southwest && d <= Northwest && (file - steps < 0)) return EMPTY_BOARD;

  // Handle special case of knight piece
  if (p == Knight && steps == 1) {
    if (d == North) return (getPieceAttack(p, s + EDGE_SIZE, East, 2) | getPieceAttack(p, s + EDGE_SIZE, West, 2));
    if (d == East) return (getPieceAttack(p, s + 1, North, 2) | getPieceAttack(p, s + 1, South, 2));
    if (d == South) return (getPieceAttack(p, s - EDGE_SIZE, East, 2) | getPieceAttack(p, s - EDGE_SIZE, West, 2));
    if (d == West) return (getPieceAttack(p, s - 1, North, 2) | getPieceAttack(p, s - 1, South, 2));
  }

  // Get attack
  Square attack;
  if (d == North) attack = s + (EDGE_SIZE * steps);
  if (d == Northeast) attack = s + (EDGE_SIZE * steps + steps);
  if (d == East) attack = s + steps;
  if (d == Southeast) attack = s - (EDGE_SIZE * steps - steps);
  if (d == South) attack = s - (EDGE_SIZE * steps);
  if (d == Southwest) attack = s - (EDGE_SIZE * steps + steps);
  if (d == West) attack = s - steps;
  if (d == Northwest) attack = s + (EDGE_SIZE * steps - steps);

  // Check attack is not vertically out of bounds
  if (attack >= BOARD_SIZE || attack < 0) return EMPTY_BOARD;

  return BitBoardSetBit(EMPTY_BOARD, attack);
}

static bool isDiagional(Direction d) {
  return d % 2 == 0;
}
