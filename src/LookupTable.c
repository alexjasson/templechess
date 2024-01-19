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

#define IS_DIAGONAL(d) (d % 2 == 1)
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
} Magic;

typedef struct {
  volatile bool stop;
  pthread_mutex_t lock;
  BitBoard *relevantBitsPowerset;
  BitBoard *moves;
  Magic magic;
} ThreadData;

struct lookupTable {
  // For each combination of square, type, color there is a corresponding move set
  // which can be indexed using the occupancies. Note that knights/kings only have 1
  // move set so they are indexed with 0. Pawns are treated the same as bishops/rooks.
  // A move set is determined by the occupancies.
  BitBoard *moves[BOARD_SIZE][TYPE_SIZE - 1][COLOR_SIZE]; // implement soon ^
  // add aquares between

  Magic magics[BOARD_SIZE][TYPE_SIZE - 1][COLOR_SIZE]; // Data used in magic hash
};

static BitBoard getMove(Square s, Type t, Direction d, int steps);
static BitBoard getLeafMoves(Square s, Type t, Color c, BitBoard occupancies);
static BitBoard *getAllPieceMoves(Square s, Type t, Color c, Magic m);

static BitBoard getRelevantBits(Square s, Type t, Color c);
static BitBoard getRelevantBitsSubset(int index, BitBoard relevantBits, int relevantBitsSize);
static Magic getMagic(Square s, Type t, Color c);
static void *magicNumberSearch(void *arg);
static int magicHash(Magic m, BitBoard occupancies);

// static int binomialCoefficient(int n, int k);
// static void generateSubsets(BitBoard currentSet, int cardinality, BitBoard* subsets, int* subsetCount, BitBoard relevantBits);
// static BitBoard* getBitSubsets(BitBoard relevantBits, int* numSets, int maxCardinality);

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
          l->magics[s][t][c] = l->magics[s][t][c - 1];
          writeElementToFile(&l->magics[s][t][c], sizeof(Magic), index, ATTACKS_DATA);
        } else if (!readElementFromFile(&l->magics[s][t][c], sizeof(Magic), index, ATTACKS_DATA)) {
          l->magics[s][t][c] = getMagic(s, t, c);
          writeElementToFile(&l->magics[s][t][c], sizeof(Magic), index, ATTACKS_DATA);
        }
        l->moves[s][t][c] = getAllPieceMoves(s, t, c, l->magics[s][t][c]);
        printf("Type: %d, Color: %d, Square: %d, Occupancy Size: %d, Magic Number:%lu\n", t, c, s, l->magics[s][t][c].relevantBitsSize, l->magics[s][t][c].magicNumber);
      }
    }
  }

  return l;
}

void LookupTableFree(LookupTable l) {
  for (Square s = a8; s <= h1; s++) {
    for (Type t = Pawn; t <= Rook; t++) {
      for (Color c = White; c <= Black; c++) {
        free(l->moves[s][t][c]);
      }
    }
  }
  free(l);
}

BitBoard LookupTableGetPieceAttacks(LookupTable l, Square s, Type t, Color c, BitBoard occupancies) {
  if (t != Queen) {
    int index = magicHash(l->magics[s][t][c], occupancies);
    return l->moves[s][t][c][index];
  } else {
    return LookupTableGetPieceAttacks(l, s, Bishop, c, occupancies) | LookupTableGetPieceAttacks(l, s, Rook, c, occupancies);
  }
}

// Return a set of all possible moves for a piece/square
static BitBoard *getAllPieceMoves(Square s, Type t, Color c, Magic m) {
  // Determine number of move sets for each piece/square


  // For Knight and King we only consider the empty set aka empty board
  int relevantBitsSetSize;
  if (t == Knight || t == King) {
    relevantBitsSetSize = 1;
  } else {
    relevantBitsSetSize = GET_POWERSET_SIZE(m.relevantBitsSize);
  }

  BitBoard *allMoves = malloc(sizeof(BitBoard) * relevantBitsSetSize);
  if (allMoves == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  if (t == Knight || t == King) {
    int index = 0;
    allMoves[index] = getLeafMoves(s, t, c, EMPTY_BOARD);
    return allMoves;
  }

  for (int i = 0; i < relevantBitsSetSize; i++) {
    BitBoard occupancies = getRelevantBitsSubset(i, m.relevantBits, m.relevantBitsSize);
    int index = magicHash(m, occupancies);
    allMoves[index] = getLeafMoves(s, t, c, occupancies);
  }

  // have a king hash later


  return allMoves;
}


/*
 * Relevant bits for Pawn: occupancies + enpassant square encoded on back rank
 * Relevant bits for Knight: empty
 * Relevant bits for King: back rank
 * Relevant bits for Bishop: occupancies
 * Relevant bits for Rook: occupancies
*/
static BitBoard getLeafMoves(Square s, Type t, Color c, BitBoard occupancies) {
  BitBoard moves = EMPTY_BOARD;

  // Loop through potential moves, if a piece can't make that move, continue to next move
  for (Direction d = North; d <= Northwest; d++) {
    for (int steps = 1; steps < EDGE_SIZE; steps++) {
      if (t == Pawn) {
        if ((steps > 1 && BitBoardGetRank(s) != 1 && c != White) ||
            (steps > 1 && BitBoardGetRank(s) != 6 && c != Black) ||
            (steps > 1 && ((c == White && d != North) || (c == Black && d != South))) ||
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
      // Continue to next direction if it's a capture, king check may not be necessary
      if (capture && t != King) break;
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

// static BitBoard getBitSets(BitBoard relevantBits, int *numSets)

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

static int magicHash(Magic m, BitBoard occupancies) {
  return (int)(((m.relevantBits & occupancies) * m.magicNumber) >> (BOARD_SIZE - m.relevantBitsSize));
}

// static int getLeafIndex(LookupTable l, Magic m, BitBoard occupancies, Type t) {
//   if (t == Knight || t == King) {
//     return 0;
//   } else {
//     return magicHash(m, occupancies);
//   }
// }

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
      int index = magicHash(m, td->relevantBitsPowerset[j]);
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


// static int binomialCoefficient(int n, int k) {
//     if (k > n - k) {
//         k = n - k;
//     }
//     int res = 1;
//     for (int i = 0; i < k; ++i) {
//         res *= (n - i);
//         res /= (i + 1);
//     }
//     return res;
// }

// static void generateSubsets(BitBoard currentSet, int cardinality, BitBoard* subsets, int* subsetCount, BitBoard relevantBits) {
//     if (cardinality == 0) {
//         subsets[(*subsetCount)++] = currentSet;
//         return;
//     }

//     BitBoard temp = relevantBits;
//     while (temp) {
//         BitBoard lowestBit = temp & -temp;
//         generateSubsets(currentSet | lowestBit, cardinality - 1, subsets, subsetCount, temp ^ lowestBit);
//         temp ^= lowestBit;
//     }
// }

// static BitBoard* getBitSubsets(BitBoard relevantBits, int* numSets, int maxCardinality) {
//     int totalBits = BitBoardCountBits(relevantBits);
//     *numSets = 0;
//     for (int i = 0; i <= maxCardinality && i <= totalBits; i++) {
//         *numSets += binomialCoefficient(totalBits, i);
//     }

//     BitBoard* subsets = malloc((*numSets) * sizeof(BitBoard));
//     if (!subsets) {
//         perror("Failed to allocate memory for subsets");
//         exit(EXIT_FAILURE);
//     }

//     int subsetCount = 0;
//     for (int i = 0; i <= maxCardinality && i <= totalBits; i++) {
//         generateSubsets(0, i, subsets, &subsetCount, relevantBits);
//     }

//     return subsets;
// }
