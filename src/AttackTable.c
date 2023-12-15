#include <stdio.h>
#include <stdlib.h>

#include "BitBoard.h"
#include "AttackTable.h"

#define WHITE_PAWN_ATTACKS {{1, -1}, {1, 1}}
#define BLACK_PAWN_ATTACKS {{-1, -1}, {-1, 1}}
#define KNIGHT_ATTACKS {{-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, -2}, {1, 2}, {2, -1}, {2, 1}}
#define KING_ATTACKS {{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}}

struct attackTable {
  BitBoard whitePawnAttacks[BOARD_SIZE];
  BitBoard blackPawnAttacks[BOARD_SIZE];
  BitBoard knightAttacks[BOARD_SIZE];
  BitBoard kingAttacks[BOARD_SIZE];
};

static BitBoard getNonSlidingPieceAttacks(Square s, Move *moves, int numMoves);

AttackTable AttackTableNew(void) {
  AttackTable a = malloc(sizeof(struct attackTable));
  if (a == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  Move whitePawnAttacks[2] = WHITE_PAWN_ATTACKS;
  Move blackPawnAttacks[2] = BLACK_PAWN_ATTACKS;
  Move knightAttacks[8] = KNIGHT_ATTACKS;
  Move kingAttacks[8] = KING_ATTACKS;

  for (Square s = a1; s <= h8; s++) {
    a->whitePawnAttacks[s] = getNonSlidingPieceAttacks(s, whitePawnAttacks, 2);
    a->blackPawnAttacks[s] = getNonSlidingPieceAttacks(s, blackPawnAttacks, 2);
    a->knightAttacks[s] = getNonSlidingPieceAttacks(s, knightAttacks, 8);
    a->kingAttacks[s] = getNonSlidingPieceAttacks(s, kingAttacks, 8);
  }

  return a;
}

void AttackTableFree(AttackTable a) {
  free(a);
}

BitBoard AttackTableGetWhitePawnAttacks(AttackTable a, Square s) {
  return a->whitePawnAttacks[s];
}

BitBoard AttackTableGetBlackPawnAttacks(AttackTable a, Square s) {
  return a->blackPawnAttacks[s];
}

BitBoard AttackTableGetKnightAttacks(AttackTable a, Square s) {
  return a->knightAttacks[s];
}

static BitBoard getNonSlidingPieceAttacks(Square s, Move *moves, int numMoves) {
  BitBoard b = 0;
  int rank = s / EDGE_SIZE;
  int file = s % EDGE_SIZE;

  for (int i = 0; i < numMoves; i++) {
    int newRank = rank + moves[i].rankOffset;
    int newFile = file + moves[i].fileOffset;

    // Check if the new position is within the board boundaries
    if (newRank >= 0 && newRank < EDGE_SIZE && newFile >= 0 && newFile < EDGE_SIZE) {
      b = BitBoardSetBit(b, newRank * EDGE_SIZE + newFile);
    }
  }

  return b;
}
