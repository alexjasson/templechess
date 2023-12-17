#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "BitBoard.h"
#include "AttackTable.h"

// Maximum size of all possible combinations of relevant occupancies for any piece
#define OCCUPANCY_POWERSET_SIZE 4096
#define PIECE_SIZE 6

struct attackTable {
  BitBoard pieceAttacks[5][2][BOARD_SIZE];
};

static BitBoard getPieceAttacks(Piece p, Color c, Square s, BitBoard occupancies);
static BitBoard getAttackSquare(Square s, Direction d, Magnitude m);

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

static BitBoard getPieceAttacks(Piece p, Color c, Square s, BitBoard occupancies) {
  if (c) printf("lol!");
  if (p != 2 && p!= 4) return 0;
  BitBoard pieceAttacks = EMPTY_BOARD;

  for (Direction d = North; d <= Northwest; d++) {
    for (Magnitude m = 1; m < EDGE_SIZE; m++) {
      if ((p == Bishop && d % 2 == 0) || (p == Rook && d % 2 == 1)) continue;
      pieceAttacks |= getAttackSquare(s, d, m);
      if (getAttackSquare(s, d, m) & occupancies) break;
    }
  }
  return pieceAttacks;
}

static BitBoard getAttackSquare(Square s, Direction d, Magnitude m) {
  if (m >= EDGE_SIZE) return EMPTY_BOARD;
  Square attack;
  int file = s % EDGE_SIZE;

  // Check out of bounds
  if (d == East && file + m >= EDGE_SIZE) return EMPTY_BOARD;
  if (d == Northeast && file + m >= EDGE_SIZE) return EMPTY_BOARD;
  if (d == Southeast && file + m >= EDGE_SIZE) return EMPTY_BOARD;
  if (d == West && file - m < 0) return EMPTY_BOARD;
  if (d == Northwest && file - m < 0) return EMPTY_BOARD;
  if (d == Southwest && file - m < 0) return EMPTY_BOARD;

  if (d == North) attack = s + (EDGE_SIZE * m);
  if (d == Northeast) attack = s + (EDGE_SIZE * m + m);
  if (d == East) attack = s + m;
  if (d == Southeast) attack = s - (EDGE_SIZE * m - m);
  if (d == South) attack = s - (EDGE_SIZE * m);
  if (d == Southwest) attack = s - (EDGE_SIZE * m + m);
  if (d == West) attack = s - m;
  if (d == Northwest) attack = s + (EDGE_SIZE * m - m);

  if (attack > 63) return EMPTY_BOARD;
  return BitBoardSetBit(EMPTY_BOARD, attack);

  fprintf(stderr, "Invalid direction!\n");
  exit(EXIT_FAILURE);
}
