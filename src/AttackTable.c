#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "BitBoard.h"
#include "AttackTable.h"

#define PIECE_QUADRANCES { (int[]){2},                        \
                           (int[]){5},                        \
                           (int[]){2, 8, 18, 32, 50, 72, 98}, \
                           (int[]){1, 2},                     \
                           (int[]){1, 4, 9, 16, 25, 36, 49}   \
                         }
#define PIECE_QUADRANCES_SIZE { 1, 1, 7, 2, 7 }
#define PIECE_SIZE 6
#define MAX_ATTACKS 14

struct attackTable {
  BitBoard pieceAttacks[5][2][BOARD_SIZE];
};

static BitBoard getPieceAttacks(Piece p, Color c, Square s);
static Vector2D* getPieceAttackVectors(Piece p, Color c);

AttackTable AttackTableNew(void) {
  AttackTable a = malloc(sizeof(struct attackTable));
  if (a == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  for (Piece p = Pawn; p <= Rook; p++) {
    for (Color c = White; c <= Black; c++) {
      for (Square s = a1; s <= h8; s++) {
        a->pieceAttacks[p][c][s] = getPieceAttacks(p, c, s);
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

static BitBoard getPieceAttacks(Piece p, Color c, Square s) {
  BitBoard b = 0;
  Vector2D *attackVectors = getPieceAttackVectors(p, c);
  int rank = s / EDGE_SIZE;
  int file = s % EDGE_SIZE;

  int i = 0;
  while (attackVectors[i].x || attackVectors[i].y) {
    int newRank = rank + attackVectors[i].y;
    int newFile = file + attackVectors[i].x;

    // Check if the new position is within the board boundaries
    if (newRank >= 0 && newRank < EDGE_SIZE && newFile >= 0 && newFile < EDGE_SIZE) {
      b = BitBoardSetBit(b, newRank * EDGE_SIZE + newFile);
    }
    i++;
  }
  free(attackVectors);

  return b;
}

// Get the attack vectors for a piece
static Vector2D* getPieceAttackVectors(Piece p, Color c) {
  Vector2D *attackVectors = calloc(MAX_ATTACKS + 100, sizeof(Vector2D));
  if (attackVectors == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  int *pieceQuadrances[] = PIECE_QUADRANCES;
  int pieceQuadrancesSize[] = PIECE_QUADRANCES_SIZE;
  int *quadrances = pieceQuadrances[p];
  int quadrancesSize = pieceQuadrancesSize[p];
  int count = 0;

  for (int i = 0; i < quadrancesSize; i++) {
    int max = (int)sqrt(quadrances[i]);
    for (int x = -max; x <= max; x++) {
      for (int y = -max; y <= max; y++) {
        if (x * x + y * y == quadrances[i] &&
            !((p == Pawn && ((c == White && y < 0) || (c == Black && y > 0))) ||
            (p == Bishop && abs(x) != abs(y)) || (p == Rook && x * y != 0))) {
          attackVectors[count++] = (Vector2D){x, y};
        }
      }
    }
  }

  return attackVectors;
}
