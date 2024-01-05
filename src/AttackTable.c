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

#define EDGES { 0x0101010101010101, 0x8080808080808080, 0x00000000000000FF, 0xFF00000000000000 }
#define NUM_EDGES 4

#define NUM_CORES sysconf(_SC_NPROCESSORS_ONLN)

#define PIECE_ATTACKS_DATA_FILEPATH "data/pieceAttacksData.dat"

typedef enum {
  North, Northeast, East, Southeast, South, Southwest, West, Northwest
} Direction;

typedef struct {
  BitBoard relevantOccupancies;
  int relevantOccupanciesSize;
  U64 magicNumber;
} PieceAttacksData;

typedef struct {
  volatile bool stop;
  pthread_mutex_t lock;
  BitBoard *attacks;
  BitBoard *occupancies;
  PieceAttacksData attacksData;
  int relevantOccupanciesPowersetSize;
} ThreadData;

struct attackTable {
  // Precalculated attack tables
  BitBoard *pieceAttacks[PIECE_SIZE];

  // Precalculated tables for indexing to the pieceAttacks hash table
  PieceAttacksData data[PIECE_SIZE];
};

static BitBoard getPieceAttacks(Piece p, BitBoard occupancies);
static BitBoard getPieceAttack(Piece p, Direction d, int steps);
static BitBoard getRelevantOccupanciesSubset(int index, int relevantOccupanciesSize, BitBoard relevantOccupancies);
static BitBoard getRelevantOccupancies(Piece p);
static U64 getMagicNumber(Piece p);
static void *magicNumberSearch(void *arg);
static int hash(PieceAttacksData data, BitBoard occupancies);
static bool isDiagional(Direction d);
static BitBoard *getAllPieceAttacks(Piece p, AttackTable a);
static int getPowersetSize(int setSize);
static PieceAttacksData getPieceAttacksData(Piece p);

AttackTable AttackTableNew(void) {
  AttackTable a = malloc(sizeof(struct attackTable));
  if (a == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  for (Piece p = 0; p < 640; p++) {
    if (!readElementFromFile(&a->data[p], sizeof(PieceAttacksData), p, PIECE_ATTACKS_DATA_FILEPATH)) {
      a->data[p] = getPieceAttacksData(p);
      writeElementToFile(&a->data[p], sizeof(PieceAttacksData), p, PIECE_ATTACKS_DATA_FILEPATH);
    }
    a->pieceAttacks[p] = getAllPieceAttacks(p, a);
    printf("Type: %d, Color: %d, Square: %d, Occupancy Size: %d, Magic Number:%lu\n", GET_TYPE(p), GET_COLOR(p), GET_SQUARE(p), a->data[p].relevantOccupanciesSize, a->data[p].magicNumber);
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
  return a->pieceAttacks[p][hash(a->data[p], occupancies)];
}

static BitBoard *getAllPieceAttacks(Piece p, AttackTable a) {
  int relevantOccupanciesPowersetSize = getPowersetSize(a->data[p].relevantOccupanciesSize);
  BitBoard *allPieceAttacks = malloc(sizeof(BitBoard) * relevantOccupanciesPowersetSize);
  if (allPieceAttacks == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < relevantOccupanciesPowersetSize; i++) {
    BitBoard o = getRelevantOccupanciesSubset(i, a->data[p].relevantOccupanciesSize, a->data[p].relevantOccupancies);
    int index = hash(a->data[p], o);
    allPieceAttacks[index] = getPieceAttacks(p, o);
  }
  return allPieceAttacks;
}

static PieceAttacksData getPieceAttacksData(Piece p) {
  PieceAttacksData data;
  data.relevantOccupancies = getRelevantOccupancies(p);
  data.relevantOccupanciesSize = BitBoardCountBits(data.relevantOccupancies);
  data.magicNumber = getMagicNumber(p);
  return data;
}

static int getPowersetSize(int setSize) {
  return 1 << setSize;
}

static BitBoard getPieceAttacks(Piece p, BitBoard occupancies) {
  BitBoard pieceAttacks = EMPTY_BOARD;
  Type t = GET_TYPE(p); Color c = GET_COLOR(p);

  // Loop through attacks, if attack does not meet criteria for piece/color then break/continue
  for (Direction d = North; d <= Northwest; d++) {
    for (int steps = 1; steps < EDGE_SIZE; steps++) {
      if ((t == Bishop && isDiagional(d)) || (t == Rook && !isDiagional(d))) continue;
      else if (t == Knight && !isDiagional(d)) break;
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

static bool isDiagional(Direction d) {
  return d % 2 == 0;
}

// get unique set of occupancies for index
static BitBoard getRelevantOccupanciesSubset(int index, int relevantOccupanciesSize, BitBoard relevantOccupancies) {
  BitBoard relevantOccupanciesSubset = EMPTY_BOARD;
  for (int i = 0; i < relevantOccupanciesSize; i++) {
    int square = BitBoardLeastSignificantBit(relevantOccupancies);
    relevantOccupancies = BitBoardPopBit(relevantOccupancies, square);
    if (index & (1 << i)) relevantOccupanciesSubset = BitBoardSetBit(relevantOccupanciesSubset, square);
  }
  return relevantOccupanciesSubset;
}

static BitBoard getRelevantOccupancies(Piece p) {
  BitBoard relevantOccupancies = getPieceAttacks(p, EMPTY_BOARD);
  BitBoard edges[NUM_EDGES] = EDGES;
  BitBoard piece = BitBoardSetBit(EMPTY_BOARD, GET_SQUARE(p));
  for (int i = 0; i < NUM_EDGES; i++) {
    if (!((piece & edges[i]) == piece)) {
      relevantOccupancies &= ~edges[i];
    }
  }
  return relevantOccupancies;
}

static int hash(PieceAttacksData data, BitBoard occupancies) {
  return (int)(((data.relevantOccupancies & occupancies) * data.magicNumber) >> (64 - data.relevantOccupanciesSize));
}

static U64 getMagicNumber(Piece p) {
  BitBoard relevantOccupancies = getRelevantOccupancies(p);
  int relevantOccupanciesSize = BitBoardCountBits(relevantOccupancies);
  int relevantOccupanciesPowersetSize = getPowersetSize(relevantOccupanciesSize);

  BitBoard *occupancies = malloc(sizeof(BitBoard) * relevantOccupanciesPowersetSize);
  BitBoard *attacks = malloc(sizeof(BitBoard) * relevantOccupanciesPowersetSize);
  if (occupancies == NULL || attacks == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < relevantOccupanciesPowersetSize; i++) {
    occupancies[i] = getRelevantOccupanciesSubset(i, relevantOccupanciesSize, relevantOccupancies);
    attacks[i] = getPieceAttacks(p, occupancies[i]);
  }

  ThreadData td = { false, PTHREAD_MUTEX_INITIALIZER, occupancies, attacks, { relevantOccupancies, relevantOccupanciesSize, UNDEFINED }, relevantOccupanciesPowersetSize };

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
  free(occupancies);
  free(attacks);
  return td.attacksData.magicNumber;
}

static void *magicNumberSearch(void *arg) {
  ThreadData *td = (ThreadData *)arg;

  BitBoard *usedAttacks = malloc(sizeof(BitBoard) * td->relevantOccupanciesPowersetSize);
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

    U64 magicNumber = getRandomNumber(&td->lock) & getRandomNumber(&td->lock) & getRandomNumber(&td->lock);

    for (int j = 0; j < td->relevantOccupanciesPowersetSize; j++) usedAttacks[j] = EMPTY_BOARD;
    int collision = false;

    // Test magic index
    for (int j = 0; j < td->relevantOccupanciesPowersetSize; j++) {
      PieceAttacksData data = { td->attacksData.relevantOccupancies, td->attacksData.relevantOccupanciesSize, magicNumber };
      int index = hash(data, td->occupancies[j]);
      if (usedAttacks[index] == EMPTY_BOARD) {
        usedAttacks[index] = td->attacks[j];
      } else if (usedAttacks[index] != td->attacks[j]) {
        collision = true;
      }
    }
    if (!collision) {
      pthread_mutex_lock(&td->lock);
      td->stop = true;
      td->attacksData.magicNumber = magicNumber;
      pthread_mutex_unlock(&td->lock);
    }
  }

  free(usedAttacks);
  return NULL;
}

