#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>

#include "BitBoard.h"
#include "LookupTable.h"
#include "utility.h"


// #define GET_1D_INDEX(s, t, c) (s * (TYPE_SIZE - 1) * COLOR_SIZE + t * COLOR_SIZE + c)
#define GET_1D_INDEX(s, t, c, d) (s * (TYPE_SIZE - 1) * COLOR_SIZE * MOVE_TYPE_SIZE + t * COLOR_SIZE * MOVE_TYPE_SIZE + c * MOVE_TYPE_SIZE + d)

#define IS_DIAGONAL(d) (d % 2 == 1)
#define GET_POWERSET_SIZE(n) (1 << n)

#define NUM_CORES sysconf(_SC_NPROCESSORS_ONLN)
#define ATTACKS_DATA "data/attacks.dat"

#define DEFAULT_COLOR White

#define MOVE_TYPE_SIZE 2

typedef enum {
  Leaf, Composite
} MoveType;

typedef enum {
  North, Northeast, East, Southeast, South, Southwest, West, Northwest
} Direction;

typedef struct {
  BitBoard relevantBits;
  int relevantBitsSize;
  uint64_t magicNumber;
} Magic;

typedef struct {
  volatile bool stop;
  pthread_mutex_t lock;
  BitBoard *relevantBitsPowerset;
  BitBoard *moves;
  Magic magic;
} ThreadData;

struct lookupTable {
  // Precalculated attack tables
  BitBoard *moves[BOARD_SIZE][TYPE_SIZE - 1][COLOR_SIZE][MOVE_TYPE_SIZE];
  Magic magics[BOARD_SIZE][TYPE_SIZE - 1][COLOR_SIZE][MOVE_TYPE_SIZE];
  // add aquares between

  // Precalculated data for indexing to the pieceAttacks hash table
};

static BitBoard getMove(Square s, Type t, Direction d, int steps);
static BitBoard getLeafMoves(Square s, Type t, Color c, BitBoard occupancies);
static BitBoard *getAllPieceMoves(Square s, Type t, Color c, Magic m);

static BitBoard getRelevantBits(Square s, Type t, Color c);
static BitBoard getRelevantBitsSubset(int index, BitBoard relevantBits, int relevantBitsSize);
static Magic getMagic(Square s, Type t, Color c);
static void *magicNumberSearch(void *arg);
static int hash(Magic m, BitBoard occupancies);

LookupTable LookupTableNew(void) {
  LookupTable l = malloc(sizeof(struct lookupTable));
  if (l == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  for (Square s = a8; s <= h1; s++) {
    for (Type t = Pawn; t <= Rook; t++) {
      for (Color c = White; c <= Black; c++) {
        for (MoveType mt = Leaf; mt <= Composite; mt++) {
          int index = GET_1D_INDEX(s, t, c, mt);
          if (mt == Composite) continue;
          if (t != Pawn && c == Black) {
            l->magics[s][t][c][mt] = l->magics[s][t][c - 1][mt];
            writeElementToFile(&l->magics[s][t][c][mt], sizeof(Magic), index, ATTACKS_DATA);
          } else if (!readElementFromFile(&l->magics[s][t][c][mt], sizeof(Magic), index, ATTACKS_DATA)) {
            l->magics[s][t][c][mt] = getMagic(s, t, c);
            writeElementToFile(&l->magics[s][t][c][mt], sizeof(Magic), index, ATTACKS_DATA);
          }
          l->moves[s][t][c][mt] = getAllPieceMoves(s, t, c, l->magics[s][t][c][mt]);
          printf("Type: %d, Color: %d, Square: %d, Occupancy Size: %d, Magic Number:%lu\n", t, c, s, l->magics[s][t][c][mt].relevantBitsSize, l->magics[s][t][c][mt].magicNumber);
        }
      }
    }
  }

  return l;
}

void LookupTableFree(LookupTable l) {
  for (Square s = a8; s <= h1; s++) {
    for (Type t = Pawn; t <= Rook; t++) {
      for (Color c = White; c <= Black; c++) {
        for (MoveType mt = Leaf; mt <= Composite; mt++)
          free(l->moves[s][t][c][mt]);
      }
    }
  }
  free(l);
}

BitBoard LookupTableGetPieceAttacks(LookupTable l, Square s, Type t, Color c, BitBoard occupancies) {
  if (t != Queen) {
    int index = hash(l->magics[s][t][c][Leaf], occupancies);
    return l->moves[s][t][c][Leaf][index];
  } else {
    return LookupTableGetPieceAttacks(l, s, Bishop, c, occupancies) | LookupTableGetPieceAttacks(l, s, Rook, c, occupancies);
  }
}

static BitBoard *getAllPieceMoves(Square s, Type t, Color c, Magic m) {
  int relevantBitsPowersetSize = GET_POWERSET_SIZE(m.relevantBitsSize);
  BitBoard *allPieceAttacks = malloc(sizeof(BitBoard) * relevantBitsPowersetSize);
  if (allPieceAttacks == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < relevantBitsPowersetSize; i++) {
    BitBoard occupancies = getRelevantBitsSubset(i, m.relevantBits, m.relevantBitsSize);
    int index = hash(m, occupancies);
    allPieceAttacks[index] = getLeafMoves(s, t, c, occupancies);
  }
  return allPieceAttacks;
}

// occupancies = relevantData - more general?
static BitBoard getLeafMoves(Square s, Type t, Color c, BitBoard occupancies) {
  BitBoard moves = EMPTY_BOARD;

  // Loop through potential moves, if a piece can't move that move, continue to next move
  for (Direction d = North; d <= Northwest; d++) {
    for (int steps = 1; steps < EDGE_SIZE; steps++) {
      if (t == Pawn) {
        if ((steps > 1 && ((c == White && d != North) || (c == Black && d != South))) ||
            (c == White && (d > Northeast && d < Northwest)) ||
            (c == Black && (d < Southeast || d > Southwest)) ||
            (steps > 2)) continue;
      } else {
        if ((t == Bishop && !IS_DIAGONAL(d)) || (t == Rook && IS_DIAGONAL(d)) ||
            (t == Knight && (IS_DIAGONAL(d) || steps > 1)) ||
            (t == King && steps > 1)) continue;
      }

      BitBoard move = getMove(s, t, d, steps);
      bool capture = move & occupancies;

      // Don't add move if it's a pawn push and capture
      if (capture && t == Pawn && !IS_DIAGONAL(d)) break;
      moves |= move;
      // Continue to next direction if it's a capture
      if (capture) break;
    }
  }
  return moves;
}

// Return a bitboard that represents a square that a piece is attacking
static BitBoard getMove(Square s, Type t, Direction d, int steps) {
    int rankOffset = (d >= Southeast && d <= Southwest) ? steps : (d <= Northeast || d == Northwest) ? -steps : 0;
    int fileOffset = (d >= Northeast && d <= Southeast) ? steps : (d >= Southwest && d <= Northwest) ? -steps : 0;
    int rank = BitBoardGetRank(s);
    int file = BitBoardGetFile(s);

    // Check for out-of-bounds conditions
    if ((rank + rankOffset >= EDGE_SIZE || rank + rankOffset < 0) ||
        (file + fileOffset >= EDGE_SIZE || file + fileOffset < 0)) {
        return EMPTY_BOARD;
    }

    if (!(t == Knight && steps == 1)) {
        return BitBoardSetBit(EMPTY_BOARD, s + EDGE_SIZE * rankOffset + fileOffset);
    }

    // Handle case where the knight hasn't finished its move
    int offset = (d == North) ? -EDGE_SIZE : (d == South) ? EDGE_SIZE : (d == East) ? 1 : -1;
    Direction d1 = (d == North || d == South) ? East : North;
    Direction d2 = (d == North || d == South) ? West : South;
    return (getMove(s + offset, t, d1, 2) | getMove(s + offset, t, d2, 2));
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
  BitBoard relevantBits = getLeafMoves(s, t, c, EMPTY_BOARD);
  BitBoard edges[NUM_EDGES] = EDGES;
  BitBoard piece = BitBoardSetBit(EMPTY_BOARD, s);
  for (int i = 0; i < NUM_EDGES; i++) {
    if (!((piece & edges[i]) == piece)) {
      relevantBits &= ~edges[i];
    }
  }
  return relevantBits;
}

static int hash(Magic m, BitBoard occupancies) {
  return (int)(((m.relevantBits & occupancies) * m.magicNumber) >> (BOARD_SIZE - m.relevantBitsSize));
}

static Magic getMagic(Square s, Type t, Color c) {
  BitBoard relevantBits = getRelevantBits(s, t, c);
  int relevantBitsSize = BitBoardCountBits(relevantBits);
  int relevantBitsPowersetSize = GET_POWERSET_SIZE(relevantBitsSize);

  BitBoard *relevantBitsPowerset = malloc(sizeof(BitBoard) * relevantBitsPowersetSize);
  BitBoard *moves = malloc(sizeof(BitBoard) * relevantBitsPowersetSize);
  if (relevantBitsPowerset == NULL || moves == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < relevantBitsPowersetSize; i++) {
    relevantBitsPowerset[i] = getRelevantBitsSubset(i, relevantBits, relevantBitsSize);
    moves[i] = getLeafMoves(s, t, c, relevantBitsPowerset[i]);
  }

  ThreadData td = { false, PTHREAD_MUTEX_INITIALIZER, relevantBitsPowerset, moves, { relevantBits, relevantBitsSize, UNDEFINED }};
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
  free(moves);
  return td.magic;
}

static void *magicNumberSearch(void *arg) {
  ThreadData *td = (ThreadData *)arg;
  int relevantBitsPowersetSize = GET_POWERSET_SIZE(td->magic.relevantBitsSize);

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
      Magic m = { td->magic.relevantBits, td->magic.relevantBitsSize, magicNumberCandidate };
      int index = hash(m, td->relevantBitsPowerset[j]);
      if (usedAttacks[index] == EMPTY_BOARD) {
        usedAttacks[index] = td->moves[j];
      } else if (usedAttacks[index] != td->moves[j]) {
        collision = true;
      }
    }
    if (!collision) {
      pthread_mutex_lock(&td->lock);
      td->stop = true;
      td->magic.magicNumber = magicNumberCandidate;
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

