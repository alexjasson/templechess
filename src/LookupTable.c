#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>

#include "BitBoard.h"
#include "LookupTable.h"
#include "utility.h"

#define PIECE_SIZE 5
#define COLOR_SIZE 2

#define IS_DIAGONAL(d) (d % 2 == 0)
#define GET_POWERSET_SIZE(n) (1 << n)
#define GET_1D_INDEX(s, p, c) (s * PIECE_SIZE * COLOR_SIZE + p * COLOR_SIZE + c)

#define EDGES { 0x0101010101010101, 0x8080808080808080, 0x00000000000000FF, 0xFF00000000000000 }
#define NUM_EDGES 4

#define NUM_CORES sysconf(_SC_NPROCESSORS_ONLN)
#define ATTACKS_DATA "data/attacks.dat"

typedef enum {
  North, Northeast, East, Southeast, South, Southwest, West, Northwest
} Direction;

typedef struct {
  BitBoard relevantBits;
  int relevantBitsSize;
  uint64_t magicNumber;
} LookupData;

typedef struct {
  volatile bool stop;
  pthread_mutex_t lock;
  BitBoard *relevantBitsPowerset;
  BitBoard *attacks;
  LookupData attacksData;
} ThreadData;

struct lookupTable {
  // Precalculated attack tables
  BitBoard *pieceAttacks[BOARD_SIZE][PIECE_SIZE][COLOR_SIZE];

  // Precalculated data for indexing to the pieceAttacks hash table
  LookupData attacksData[BOARD_SIZE][PIECE_SIZE][COLOR_SIZE];
};

static BitBoard getPieceAttack(Square s, Piece p, Direction d, int steps);
static BitBoard getPieceAttacks(Square s, Piece p, Color c, BitBoard occupancies);
static BitBoard *getAllPieceAttacks(Square s, Piece p, Color c, LookupData attacks);
static LookupData getPieceAttacksData(Square s, Piece p, Color c);

static BitBoard getRelevantBits(Square s, Piece p, Color c);
static BitBoard getRelevantBitsSubset(int index, BitBoard relevantBits, int relevantBitsSize);
static uint64_t getMagicNumber(Square s, Piece p, Color c, BitBoard relevantBits, int relevantBitsSize);
static void *magicNumberSearch(void *arg);
static int hash(LookupData attacks, BitBoard occupancies);

LookupTable LookupTableNew(void) {
  LookupTable l = malloc(sizeof(struct lookupTable));
  if (l == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  for (Square s = a1; s <= h8; s++) {
    for (Piece p = Pawn; p <= Rook; p++) {
      for (Color c = White; c <= Black; c++) {
        int index = GET_1D_INDEX(s, p, c);
        if (p != Pawn && c == Black) {
          l->attacksData[s][p][c] = l->attacksData[s][p][c - 1];
          writeElementToFile(&l->attacksData[s][p][c], sizeof(LookupData), index, ATTACKS_DATA);
        } else if (!readElementFromFile(&l->attacksData[s][p][c], sizeof(LookupData), index, ATTACKS_DATA)) {
          l->attacksData[s][p][c] = getPieceAttacksData(s, p, c);
          writeElementToFile(&l->attacksData[s][p][c], sizeof(LookupData), index, ATTACKS_DATA);
        }
        l->pieceAttacks[s][p][c] = getAllPieceAttacks(s, p, c, l->attacksData[s][p][c]);
        printf("Type: %d, Color: %d, Square: %d, Occupancy Size: %d, Magic Number:%lu\n", p, c, s, l->attacksData[s][p][c].relevantBitsSize, l->attacksData[s][p][c].magicNumber);
      }
    }
  }

  return l;
}

void LookupTableFree(LookupTable l) {
  for (Square s = a1; s <= h8; s++) {
    for (Piece p = Pawn; p <= Rook; p++) {
      for (Color c = White; c <= Black; c++) {
        free(l->pieceAttacks[s][p][c]);
      }
    }
  }
  free(l);
}

BitBoard LookupTableGetPieceAttacks(LookupTable l, Square s, Piece p, Color c, BitBoard occupancies) {
  if (p != Queen) {
    int index = hash(l->attacksData[s][p][c], occupancies);
    return l->pieceAttacks[s][p][c][index];
  } else {
    return LookupTableGetPieceAttacks(l, s, Bishop, c, occupancies) | LookupTableGetPieceAttacks(l, s, Rook, c, occupancies);
  }
}

static BitBoard *getAllPieceAttacks(Square s, Piece p, Color c, LookupData attacks) {
  int relevantBitsPowersetSize = GET_POWERSET_SIZE(attacks.relevantBitsSize);
  BitBoard *allPieceAttacks = malloc(sizeof(BitBoard) * relevantBitsPowersetSize);
  if (allPieceAttacks == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < relevantBitsPowersetSize; i++) {
    BitBoard occupancies = getRelevantBitsSubset(i, attacks.relevantBits, attacks.relevantBitsSize);
    int index = hash(attacks, occupancies);
    allPieceAttacks[index] = getPieceAttacks(s, p, c, occupancies);
  }
  return allPieceAttacks;
}

static LookupData getPieceAttacksData(Square s, Piece p, Color c) {
  LookupData attacks;
  attacks.relevantBits = getRelevantBits(s, p, c);
  attacks.relevantBitsSize = BitBoardCountBits(attacks.relevantBits);
  attacks.magicNumber = getMagicNumber(s, p, c, attacks.relevantBits, attacks.relevantBitsSize);
  return attacks;
}

static BitBoard getPieceAttacks(Square s, Piece p, Color c, BitBoard occupancies) {
  BitBoard pieceAttacks = EMPTY_BOARD;

  // Loop through attacks, if attack does not meet criteria for piece/color then break/continue
  for (Direction d = North; d <= Northwest; d++) {
    for (int steps = 1; steps < EDGE_SIZE; steps++) {
      if ((p == Bishop && IS_DIAGONAL(d)) || (p == Rook && !IS_DIAGONAL(d))) continue;
      else if (p == Knight && !IS_DIAGONAL(d)) break;
      else if (p <= King && steps > 1) break;
      else if (p == Pawn && c == White && d != Northeast && d != Northwest) break;
      else if (p == Pawn && c == Black && d != Southeast && d != Southwest) break;

      pieceAttacks |= getPieceAttack(s, p, d, steps);
      if (getPieceAttack(s, p, d, steps) & occupancies) break;
    }
  }
  return pieceAttacks;
}

// Return a bitboard that represents a square that a piece is attacking
static BitBoard getPieceAttack(Square s, Piece p, Direction d, int steps) {
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
  return (getPieceAttack(s + offset, p, d1, 2) | getPieceAttack(s + offset, p, d2, 2));
}

static BitBoard getRelevantBitsSubset(int index, BitBoard relevantBits, int relevantBitsSize) {
  BitBoard relevantBitsSubset = EMPTY_BOARD;
  for (int i = 0; i < relevantBitsSize; i++) {
    int square = BitBoardLeastSignificantBit(relevantBits);
    relevantBits = BitBoardPopBit(relevantBits, square);
    if (index & (1 << i)) relevantBitsSubset = BitBoardSetBit(relevantBitsSubset, square);
  }
  return relevantBitsSubset;
}

static BitBoard getRelevantBits(Square s, Piece p, Color c) {
  BitBoard relevantBits = getPieceAttacks(s, p, c, EMPTY_BOARD);
  BitBoard edges[NUM_EDGES] = EDGES;
  BitBoard piece = BitBoardSetBit(EMPTY_BOARD, s);
  for (int i = 0; i < NUM_EDGES; i++) {
    if (!((piece & edges[i]) == piece)) {
      relevantBits &= ~edges[i];
    }
  }
  return relevantBits;
}

static int hash(LookupData attacks, BitBoard occupancies) {
  return (int)(((attacks.relevantBits & occupancies) * attacks.magicNumber) >> (64 - attacks.relevantBitsSize));
}

static uint64_t getMagicNumber(Square s, Piece p, Color c, BitBoard relevantBits, int relevantBitsSize) {
  int relevantBitsPowersetSize = GET_POWERSET_SIZE(relevantBitsSize);

  BitBoard *relevantBitsPowerset = malloc(sizeof(BitBoard) * relevantBitsPowersetSize);
  BitBoard *attacks = malloc(sizeof(BitBoard) * relevantBitsPowersetSize);
  if (relevantBitsPowerset == NULL || attacks == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < relevantBitsPowersetSize; i++) {
    relevantBitsPowerset[i] = getRelevantBitsSubset(i, relevantBits, relevantBitsSize);
    attacks[i] = getPieceAttacks(s, p, c, relevantBitsPowerset[i]);
  }

  ThreadData td = { false, PTHREAD_MUTEX_INITIALIZER, relevantBitsPowerset, attacks, { relevantBits, relevantBitsSize, UNDEFINED }};
  int numCores = NUM_CORES;

  // create threads
  pthread_t threads[numCores];
  for (int i = 0; i < numCores ; i++) {
    pthread_create(&threads[i], NULL, magicNumberSearch, &td);
  }

  // join threads
  for (int i = 0; i < numCores ; i++) {
    pthread_join(threads[i], NULL);
  }

  pthread_mutex_destroy(&td.lock);
  free(relevantBitsPowerset);
  free(attacks);
  return td.attacksData.magicNumber;
}

static void *magicNumberSearch(void *arg) {
  ThreadData *td = (ThreadData *)arg;
  int relevantBitsPowersetSize = GET_POWERSET_SIZE(td->attacksData.relevantBitsSize);

  BitBoard *usedAttacks = malloc(sizeof(BitBoard) * relevantBitsPowersetSize);
  if (usedAttacks == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  while(true) {
    pthread_mutex_lock(&td->lock);
    if (td->stop) {
      pthread_mutex_unlock(&td->lock);
      break;
    }
    pthread_mutex_unlock(&td->lock);

    uint64_t magicNumberCandidate = getRandomNumber(&td->lock) & getRandomNumber(&td->lock) & getRandomNumber(&td->lock);

    for (int j = 0; j < relevantBitsPowersetSize; j++) usedAttacks[j] = EMPTY_BOARD;
    int collision = false;

    // Test magic index
    for (int j = 0; j < relevantBitsPowersetSize; j++) {
      LookupData attacks = { td->attacksData.relevantBits, td->attacksData.relevantBitsSize, magicNumberCandidate };
      int index = hash(attacks, td->relevantBitsPowerset[j]);
      if (usedAttacks[index] == EMPTY_BOARD) {
        usedAttacks[index] = td->attacks[j];
      } else if (usedAttacks[index] != td->attacks[j]) {
        collision = true;
      }
    }
    if (!collision) {
      pthread_mutex_lock(&td->lock);
      td->stop = true;
      td->attacksData.magicNumber = magicNumberCandidate;
      pthread_mutex_unlock(&td->lock);
    }
  }

  free(usedAttacks);
  return NULL;
}

