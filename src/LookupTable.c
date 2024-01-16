#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>

#include "BitBoard.h"
#include "LookupTable.h"
#include "utility.h"

#define GET_1D_INDEX(s, t, c) (s * (TYPE_SIZE - 1) * COLOR_SIZE + t * COLOR_SIZE + c)

#define IS_DIAGONAL(d) (d % 2 == 0)
#define GET_POWERSET_SIZE(n) (1 << n)

#define NUM_CORES sysconf(_SC_NPROCESSORS_ONLN)
#define ATTACKS_DATA "data/attacks.dat"

#define DEFAULT_COLOR White

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
  BitBoard *pieceAttacks[BOARD_SIZE][TYPE_SIZE - 1][COLOR_SIZE];
  // add aquares between

  // Precalculated data for indexing to the pieceAttacks hash table
  LookupData attacksData[BOARD_SIZE][TYPE_SIZE - 1][COLOR_SIZE];
};

static BitBoard getPieceAttack(Square s, Type t, Direction d, int steps);
static BitBoard getPieceAttacks(Square s, Type t, Color c, BitBoard occupancies);
static BitBoard *getAllPieceAttacks(Square s, Type t, Color c, LookupData attacks);
static LookupData getPieceAttacksData(Square s, Type t, Color c);

static BitBoard getRelevantBits(Square s, Type t, Color c);
static BitBoard getRelevantBitsSubset(int index, BitBoard relevantBits, int relevantBitsSize);
static uint64_t getMagicNumber(Square s, Type t, Color c, BitBoard relevantBits, int relevantBitsSize);
static void *magicNumberSearch(void *arg);
static int hash(LookupData attacks, BitBoard occupancies);

LookupTable LookupTableNew(void) {
  LookupTable l = malloc(sizeof(struct lookupTable));
  if (l == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  for (Square s = a8; s <= h1; s++) {
    for (Type t = Pawn; t <= Rook; t++) {
      for (Color c = White; c <= Black; c++) {
        int index = GET_1D_INDEX(s, t, c);
        if (t != Pawn && c == Black) {
          l->attacksData[s][t][c] = l->attacksData[s][t][c - 1];
          writeElementToFile(&l->attacksData[s][t][c], sizeof(LookupData), index, ATTACKS_DATA);
        } else if (!readElementFromFile(&l->attacksData[s][t][c], sizeof(LookupData), index, ATTACKS_DATA)) {
          l->attacksData[s][t][c] = getPieceAttacksData(s, t, c);
          writeElementToFile(&l->attacksData[s][t][c], sizeof(LookupData), index, ATTACKS_DATA);
        }
        l->pieceAttacks[s][t][c] = getAllPieceAttacks(s, t, c, l->attacksData[s][t][c]);
        printf("Type: %d, Color: %d, Square: %d, Occupancy Size: %d, Magic Number:%lu\n", t, c, s, l->attacksData[s][t][c].relevantBitsSize, l->attacksData[s][t][c].magicNumber);
      }
    }
  }

  return l;
}

void LookupTableFree(LookupTable l) {
  for (Square s = a8; s <= h1; s++) {
    for (Type t = Pawn; t <= Rook; t++) {
      for (Color c = White; c <= Black; c++) {
        free(l->pieceAttacks[s][t][c]);
      }
    }
  }
  free(l);
}

BitBoard LookupTableGetPieceAttacks(LookupTable l, Square s, Type t, Color c, BitBoard occupancies) {
  if (t != Queen) {
    int index = hash(l->attacksData[s][t][c], occupancies);
    return l->pieceAttacks[s][t][c][index];
  } else {
    return LookupTableGetPieceAttacks(l, s, Bishop, c, occupancies) | LookupTableGetPieceAttacks(l, s, Rook, c, occupancies);
  }
}

static BitBoard *getAllPieceAttacks(Square s, Type t, Color c, LookupData attacks) {
  int relevantBitsPowersetSize = GET_POWERSET_SIZE(attacks.relevantBitsSize);
  BitBoard *allPieceAttacks = malloc(sizeof(BitBoard) * relevantBitsPowersetSize);
  if (allPieceAttacks == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < relevantBitsPowersetSize; i++) {
    BitBoard occupancies = getRelevantBitsSubset(i, attacks.relevantBits, attacks.relevantBitsSize);
    int index = hash(attacks, occupancies);
    allPieceAttacks[index] = getPieceAttacks(s, t, c, occupancies);
  }
  return allPieceAttacks;
}

static LookupData getPieceAttacksData(Square s, Type t, Color c) {
  LookupData attacks;
  attacks.relevantBits = getRelevantBits(s, t, c);
  attacks.relevantBitsSize = BitBoardCountBits(attacks.relevantBits);
  attacks.magicNumber = getMagicNumber(s, t, c, attacks.relevantBits, attacks.relevantBitsSize);
  return attacks;
}

// occupancies = relevantData - more general?
static BitBoard getPieceAttacks(Square s, Type t, Color c, BitBoard occupancies) {
  BitBoard pieceAttacks = EMPTY_BOARD;

  // Loop through attacks, if attack does not meet criteria for piece/color then break/continue
  for (Direction d = North; d <= Northwest; d++) {
    for (int steps = 1; steps < EDGE_SIZE; steps++) {
      if ((t == Bishop && IS_DIAGONAL(d)) || (t == Rook && !IS_DIAGONAL(d))) continue;
      else if (t == Knight && !IS_DIAGONAL(d)) break;
      else if (t <= King && steps > 1) break;
      else if (t == Pawn && c == White && d != Northeast && d != Northwest) break;
      else if (t == Pawn && c == Black && d != Southeast && d != Southwest) break;

      pieceAttacks |= getPieceAttack(s, t, d, steps);
      if (getPieceAttack(s, t, d, steps) & occupancies) break;
    }
  }
  return pieceAttacks;
}

// Return a bitboard that represents a square that a piece is attacking
static BitBoard getPieceAttack(Square s, Type t, Direction d, int steps) {
  // Check out of bounds conditions
  int rankOffset = d >= Southeast && d <= Southwest ? steps : d <= Northeast || d == Northwest ? -steps : 0;
  int fileOffset = d >= Northeast && d <= Southeast ? steps : d >= Southwest && d <= Northwest ? -steps : 0;
  int rank = BitBoardGetRank(s);
  int file = BitBoardGetFile(s);
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
  return (getPieceAttack(s + offset, t, d1, 2) | getPieceAttack(s + offset, t, d2, 2));
}

static BitBoard getRelevantBitsSubset(int index, BitBoard relevantBits, int relevantBitsSize) {
  BitBoard relevantBitsSubset = EMPTY_BOARD;
  for (int i = 0; i < relevantBitsSize; i++) {
    Square s = BitBoardLeastSignificantBit(relevantBits);
    relevantBits = BitBoardPopBit(relevantBits, s);
    if (index & (1 << i)) relevantBitsSubset = BitBoardSetBit(relevantBitsSubset, s);
  }
  return relevantBitsSubset;
}

static BitBoard getRelevantBits(Square s, Type t, Color c) {
  BitBoard relevantBits = getPieceAttacks(s, t, c, EMPTY_BOARD);
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
  return (int)(((attacks.relevantBits & occupancies) * attacks.magicNumber) >> (BOARD_SIZE - attacks.relevantBitsSize));
}

static uint64_t getMagicNumber(Square s, Type t, Color c, BitBoard relevantBits, int relevantBitsSize) {
  int relevantBitsPowersetSize = GET_POWERSET_SIZE(relevantBitsSize);

  BitBoard *relevantBitsPowerset = malloc(sizeof(BitBoard) * relevantBitsPowersetSize);
  BitBoard *attacks = malloc(sizeof(BitBoard) * relevantBitsPowersetSize);
  if (relevantBitsPowerset == NULL || attacks == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < relevantBitsPowersetSize; i++) {
    relevantBitsPowerset[i] = getRelevantBitsSubset(i, relevantBits, relevantBitsSize);
    attacks[i] = getPieceAttacks(s, t, c, relevantBitsPowerset[i]);
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

// BitBoard getSquaresBetween(LookupTable l, Square s1, Square s2) {
//   BitBoard squaresBetween;
//   for (Square s1 = a8; s1 <= h1; s1++) {
//     for (Square s2 = a8; s2 <= h1; s2++) {
//       squaresBetween = BitBoardSetBit(EMPTY_BOARD, s1) | BitBoardSetBit(EMPTY_BOARD, s2);
//       if (BitBoardGetFile(s1) == BitBoardGetFile(s2) || BitBoardGetRank(s1) == BitBoardGetRank(s2)) {
//         squaresBetween = LookupTableGetPieceAttacks(l, s1, Rook, DEFAULT_COLOR, squaresBetween) &
//                          LookupTableGetPieceAttacks(l, s2, Rook, DEFAULT_COLOR, squaresBetween);
//       }
//     }
//   }
// }

