#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>

#include "BitBoard.h"
#include "AttackTable.h"
#include "utility.h"

#define TYPE_SIZE 6
#define COLOR_SIZE 2
#define PIECE_SIZE 768

#define GET_PIECE(t, c, s) ((t << 7) + (c << 6) + s)
#define GET_TYPE(p) ((p >> 7) % 6)
#define GET_COLOR(p) ((p >> 6) & 1)
#define GET_SQUARE(p) (p & 63)
#define IS_DIAGONAL(d) (d % 2 == 0)
#define GET_POWERSET_SIZE(n) (1 << n)

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
  U64 magicNumber;
} PieceData;

typedef struct {
  volatile bool stop;
  pthread_mutex_t lock;
  BitBoard *relevantBitsPowerset;
  BitBoard *attacks;
  PieceData attacksData;
} ThreadData;

struct attackTable {
  // Precalculated attack tables
  BitBoard *pieceAttacks[PIECE_SIZE];

  // Precalculated data for indexing to the pieceAttacks hash table
  PieceData attacksData[PIECE_SIZE];
};

static BitBoard getPieceAttack(Piece p, Direction d, int steps);
static BitBoard getPieceAttacks(Piece p, BitBoard occupancies);
static BitBoard *getAllPieceAttacks(Piece p, PieceData attacks);
static PieceData getPieceAttacksData(Piece p);

static BitBoard getRelevantBits(Piece p);
static BitBoard getRelevantBitsSubset(int index, BitBoard relevantBits, int relevantBitsSize);
static U64 getMagicNumber(Piece p, BitBoard relevantBits, int relevantBitsSize);
static void *magicNumberSearch(void *arg);
static int hash(PieceData attacks, BitBoard occupancies);

AttackTable AttackTableNew(void) {
  AttackTable a = malloc(sizeof(struct attackTable));
  if (a == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  // Assumes that data will be read/written consecutively
  for (Piece p = 0; p < 640; p++) {
    if (p != Pawn && GET_COLOR(p) == Black) {
      a->attacksData[p] = a->attacksData[p - BOARD_SIZE];
      writeElementToFile(&a->attacksData[p], sizeof(PieceData), p, ATTACKS_DATA);
    } else if (!readElementFromFile(&a->attacksData[p], sizeof(PieceData), p, ATTACKS_DATA)) {
      a->attacksData[p] = getPieceAttacksData(p);
      writeElementToFile(&a->attacksData[p], sizeof(PieceData), p, ATTACKS_DATA);
    }
    a->pieceAttacks[p] = getAllPieceAttacks(p, a->attacksData[p]);
    printf("Type: %d, Color: %d, Square: %d, Occupancy Size: %d, Magic Number:%lu\n", GET_TYPE(p), GET_COLOR(p), GET_SQUARE(p), a->attacksData[p].relevantBitsSize, a->attacksData[p].magicNumber);
  }
  return a;
}

void AttackTableFree(AttackTable a) {
  for (Piece p = 0; p < PIECE_SIZE; p++) {
    free(a->pieceAttacks[p]);
  }
  free(a);
}

BitBoard AttackTableGetPieceAttacks(AttackTable a, Piece p, BitBoard occupancies) {
  int index = hash(a->attacksData[p], occupancies);
  return a->pieceAttacks[p][index];
}

static BitBoard *getAllPieceAttacks(Piece p, PieceData attacks) {
  int relevantBitsPowersetSize = GET_POWERSET_SIZE(attacks.relevantBitsSize);
  BitBoard *allPieceAttacks = malloc(sizeof(BitBoard) * relevantBitsPowersetSize);
  if (allPieceAttacks == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < relevantBitsPowersetSize; i++) {
    BitBoard occupancies = getRelevantBitsSubset(i, attacks.relevantBits, attacks.relevantBitsSize);
    int index = hash(attacks, occupancies);
    allPieceAttacks[index] = getPieceAttacks(p, occupancies);
  }
  return allPieceAttacks;
}

static PieceData getPieceAttacksData(Piece p) {
  PieceData attacks;
  attacks.relevantBits = getRelevantBits(p);
  attacks.relevantBitsSize = BitBoardCountBits(attacks.relevantBits);
  attacks.magicNumber = getMagicNumber(p, attacks.relevantBits, attacks.relevantBitsSize);
  return attacks;
}

static BitBoard getPieceAttacks(Piece p, BitBoard occupancies) {
  BitBoard pieceAttacks = EMPTY_BOARD;
  Type t = GET_TYPE(p); Color c = GET_COLOR(p);

  // Loop through attacks, if attack does not meet criteria for piece/color then break/continue
  for (Direction d = North; d <= Northwest; d++) {
    for (int steps = 1; steps < EDGE_SIZE; steps++) {
      if ((t == Bishop && IS_DIAGONAL(d)) || (t == Rook && !IS_DIAGONAL(d))) continue;
      else if (t == Knight && !IS_DIAGONAL(d)) break;
      else if (t <= King && steps > 1) break;
      else if (t == Pawn && c == White && d != Northeast && d != Northwest) break;
      else if (t == Pawn && c == Black && d != Southeast && d != Southwest) break;

      pieceAttacks |= getPieceAttack(p, d, steps);
      if (getPieceAttack(p, d, steps) & occupancies) break;
    }
  }
  return pieceAttacks;
}

// Return a bitboard that represents a square that a piece is attacking
static BitBoard getPieceAttack(Piece p, Direction d, int steps) {
  // Check out of bounds conditions
  Type t = GET_TYPE(p); Color c = GET_COLOR(p); Square s = GET_SQUARE(p);
  int rankOffset = d <= Northeast || d == Northwest ? steps : d >= Southeast && d <= Southwest ? -steps : 0;
  int fileOffset = d >= Northeast && d <= Southeast ? steps : d >= Southwest && d <= Northwest ? -steps : 0;
  int rank = s / EDGE_SIZE;
  int file = s % EDGE_SIZE;
  if ((rank + rankOffset >= EDGE_SIZE || rank + rankOffset < 0) ||
      (file + fileOffset >= EDGE_SIZE || file + fileOffset < 0)) {
    return EMPTY_BOARD;
  }

  if (!(t == Knight && steps == 1)) {
    return BitBoardSetBit(EMPTY_BOARD, s + EDGE_SIZE * rankOffset + fileOffset);
  }

  // Handle case where knight hasn't finished it's move
  int offset = (d == North) ? EDGE_SIZE : (d == South) ? -EDGE_SIZE : (d == East) ? 1 : -1;
  Direction d1 = (d == North || d == South) ? East : North;
  Direction d2 = (d == North || d == South) ? West : South;
  p = GET_PIECE(t, c, s + offset);
  return (getPieceAttack(p, d1, 2) | getPieceAttack(p, d2, 2));
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

static BitBoard getRelevantBits(Piece p) {
  BitBoard relevantBits = getPieceAttacks(p, EMPTY_BOARD);
  BitBoard edges[NUM_EDGES] = EDGES;
  BitBoard piece = BitBoardSetBit(EMPTY_BOARD, GET_SQUARE(p));
  for (int i = 0; i < NUM_EDGES; i++) {
    if (!((piece & edges[i]) == piece)) {
      relevantBits &= ~edges[i];
    }
  }
  return relevantBits;
}

static int hash(PieceData attacks, BitBoard occupancies) {
  return (int)(((attacks.relevantBits & occupancies) * attacks.magicNumber) >> (64 - attacks.relevantBitsSize));
}

static U64 getMagicNumber(Piece p, BitBoard relevantBits, int relevantBitsSize) {
  int relevantBitsPowersetSize = GET_POWERSET_SIZE(relevantBitsSize);

  BitBoard *relevantBitsPowerset = malloc(sizeof(BitBoard) * relevantBitsPowersetSize);
  BitBoard *attacks = malloc(sizeof(BitBoard) * relevantBitsPowersetSize);
  if (relevantBitsPowerset == NULL || attacks == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < relevantBitsPowersetSize; i++) {
    relevantBitsPowerset[i] = getRelevantBitsSubset(i, relevantBits, relevantBitsSize);
    attacks[i] = getPieceAttacks(p, relevantBitsPowerset[i]);
  }

  ThreadData td = { false, PTHREAD_MUTEX_INITIALIZER, relevantBitsPowerset, attacks, { relevantBits, relevantBitsSize, UNDEFINED }};

  // create threads
  pthread_t threads[NUM_CORES];
  for (int i = 0; i < NUM_CORES; i++) {
    pthread_create(&threads[i], NULL, magicNumberSearch, &td);
  }

  // join threads
  for (int i = 0; i < NUM_CORES; i++) {
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

    U64 magicNumberCandidate = getRandomNumber(&td->lock) & getRandomNumber(&td->lock) & getRandomNumber(&td->lock);

    for (int j = 0; j < relevantBitsPowersetSize; j++) usedAttacks[j] = EMPTY_BOARD;
    int collision = false;

    // Test magic index
    for (int j = 0; j < relevantBitsPowersetSize; j++) {
      PieceData attacks = { td->attacksData.relevantBits, td->attacksData.relevantBitsSize, magicNumberCandidate };
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

