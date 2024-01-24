#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>

#include "BitBoard.h"
#include "LookupTable.h"
#include "utility.h"

#define IS_DIAGONAL(d) (d % 2 == 1)
#define GET_POWERSET_SIZE(n) (1 << n)

#define NUM_CORES sysconf(_SC_NPROCESSORS_ONLN)
#define MAGICS_FILEPATH "data/magics.dat"

#define DEFAULT_COLOR White

typedef enum {
  North, Northeast, East, Southeast, South, Southwest, West, Northwest
} Direction;

typedef struct {
  BitBoard relevantBits;
  int relevantBitsSize;
  uint64_t magicNumber;
} Magic;

// For king:
// bits = bottom rank
// bitShift = 0 for black, 8 for white - so not same as usual magic
// magicNumber = 1

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
  BitBoard *attacks[BOARD_SIZE][TYPE_SIZE - 1][COLOR_SIZE]; // implement soon ^
  BitBoard *moves[BOARD_SIZE][TYPE_SIZE - 1][COLOR_SIZE];
  // add aquares between

  Magic magics[BOARD_SIZE][TYPE_SIZE - 1][COLOR_SIZE]; // Data used in magic hash
};

static BitBoard getMove(Square s, Type t, Direction d, int steps);
static BitBoard getAttacks(Square s, Type t, Color c, BitBoard occupancies);
static BitBoard getAttackBits(Square s, Type t, Color c);
static BitBoard *getAllAttacks(Square s, Type t, Color c, Magic m);

static BitBoard getMoves(Square s, Type t, Color c, BitBoard occupancies);
static BitBoard getMoveBits(Square s, Type t, Color c);
static BitBoard *getAllMoves(Square s, Type t, Color c, Magic m, LookupTable l);

static BitBoard getBitsSubset(int index, BitBoard relevantBits, int relevantBitsSize);
static Magic getMagic(Square s, Type t, Color c);
static void *magicNumberSearch(void *arg);
static int magicHash(Magic m, BitBoard occupancies);

LookupTable LookupTableNew(void) {
  LookupTable l = malloc(sizeof(struct lookupTable));
  if (l == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  bool emptyFile = isFileEmpty(MAGICS_FILEPATH);
  int fileElementsSize = BOARD_SIZE * (TYPE_SIZE - 1) * COLOR_SIZE;
  if (!emptyFile) readFromFile(l->magics, sizeof(Magic), fileElementsSize, MAGICS_FILEPATH);

  for (Square s = a8; s <= h1; s++) {
    for (Type t = Pawn; t <= Rook; t++) {
      for (Color c = White; c <= Black; c++) {
        if (emptyFile) l->magics[s][t][c] = getMagic(s, t, c);
        l->attacks[s][t][c] = getAllAttacks(s, t, c, l->magics[s][t][c]);
        l->moves[s][t][c] = getAllMoves(s, t, c, l->magics[s][t][c], l);
        printf("Type: %d, Color: %d, Square: %d, Occupancy Size: %d, Magic Number:%lu\n", t, c, s, l->magics[s][t][c].relevantBitsSize, l->magics[s][t][c].magicNumber);
      }
    }
  }

  if (emptyFile) writeToFile(l->magics, sizeof(Magic), fileElementsSize, MAGICS_FILEPATH);

  return l;
}

void LookupTableFree(LookupTable l) {
  for (Square s = a8; s <= h1; s++) {
    for (Type t = Pawn; t <= Rook; t++) {
      for (Color c = White; c <= Black; c++) {
        free(l->attacks[s][t][c]);
        if (t == Pawn) free(l->moves[s][t][c]);
      }
    }
  }
  free(l);
}

BitBoard LookupTableGetPieceAttacks(LookupTable l, Square s, Type t, Color c, BitBoard occupancies) {
  if (t <= Knight) {
    return l->attacks[s][t][c][0];
  } else if (t != Queen) {
    int index = magicHash(l->magics[s][t][c], occupancies);
    return l->attacks[s][t][c][index];
  } else {
    return LookupTableGetPieceAttacks(l, s, Bishop, c, occupancies) | LookupTableGetPieceAttacks(l, s, Rook, c, occupancies);
  }
}

BitBoard LookupTableGetMoves(LookupTable l, Square s, Type t, Color c, BitBoard occupancies) {
  if (t == King || t == Knight) { // Will only be Knight in future
    return l->moves[s][t][c][0];
  } else if (t != Queen) {
    int index = magicHash(l->magics[s][t][c], occupancies);
    return l->moves[s][t][c][index];
  } else {
    return LookupTableGetMoves(l, s, Bishop, c, occupancies) | LookupTableGetMoves(l, s, Rook, c, occupancies);
  }
}

// Return a set of all possible attacks for a piece/square
static BitBoard *getAllAttacks(Square s, Type t, Color c, Magic m) {
  // For Pawns, knight and king there is only 1 attack set for each square
  int numSets = (t <= Knight) ? 1 : GET_POWERSET_SIZE(m.relevantBitsSize);

  BitBoard *allAttacks = malloc(sizeof(BitBoard) * numSets);
  if (allAttacks == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  if (t <= Knight) {
    allAttacks[0] = getAttacks(s, t, c, EMPTY_BOARD);
  } else {
    for (int i = 0; i < numSets; i++) {
      BitBoard occupancies = getBitsSubset(i, m.relevantBits, m.relevantBitsSize);
      int index = magicHash(m, occupancies);
      allAttacks[index] = getAttacks(s, t, c, occupancies);
    }
  }

  return allAttacks;
}

static BitBoard *getAllMoves(Square s, Type t, Color c, Magic m, LookupTable l) {
  if (t != Pawn) return l->attacks[s][t][c];

  int numSets = GET_POWERSET_SIZE(m.relevantBitsSize);

  BitBoard *allMoves = malloc(sizeof(BitBoard) * numSets);
  if (allMoves == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < numSets; i++) {
    BitBoard occupancies = getBitsSubset(i, m.relevantBits, m.relevantBitsSize);
    int index = magicHash(m, occupancies);
    allMoves[index] = getMoves(s, t, c, occupancies);
  }

  return allMoves;
}


/*
 * Relevant bits for Pawn: occupancies + enpassant square encoded on back rank
 * Relevant bits for Knight: empty
 * Relevant bits for King: back rank
 * Relevant bits for Bishop: occupancies
 * Relevant bits for Rook: occupancies
*/
static BitBoard getAttacks(Square s, Type t, Color c, BitBoard occupancies) {
  BitBoard attacks = EMPTY_BOARD;

  // Loop through attacks, if attack does not meet criteria for piece/color then break/continue
  for (Direction d = North; d <= Northwest; d++) {
    for (int steps = 1; steps < EDGE_SIZE; steps++) {
      if ((t == Bishop && !IS_DIAGONAL(d)) || (t == Rook && IS_DIAGONAL(d))) continue;
      else if (t == Knight && IS_DIAGONAL(d)) break;
      else if (t <= Knight && steps > 1) break;
      else if (t == Pawn && c == White && d != Northeast && d != Northwest) break;
      else if (t == Pawn && c == Black && d != Southeast && d != Southwest) break;

      BitBoard attack = getMove(s, t, d, steps);
      bool capture = attack & occupancies;
      attacks |= attack;
      if (capture) break;
    }
  }
  return attacks;
}

// The same as attacks except pawns cant move to empty squares diagonally
// and we add pawn pushes
static BitBoard getMoves(Square s, Type t, Color c, BitBoard occupancies) {
  BitBoard moves = getAttacks(s, t, c, occupancies);
  // Return early if piece isn't pawn
  if (t != Pawn) return moves;

  // Remove any of the pawn attacks if the square is empty since they are not moves
  moves &= occupancies;
  bool initialRank = (c == White) ? BitBoardGetRank(s) == 6 : BitBoardGetRank(s) == 1;
  for (int steps = 1; steps <= 2; steps++) {
    Direction d = (c == White) ? North : South;
    BitBoard move = getMove(s, t, d, steps);
    bool capture = move & occupancies;
    if (capture) break;
    if (steps == 2 && !initialRank) break;
    moves |= move;
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

static BitBoard getBitsSubset(int index, BitBoard relevantBits, int relevantBitsSize) {
  BitBoard relevantBitsSubset = EMPTY_BOARD;
  for (int i = 0; i < relevantBitsSize; i++) {
    Square s = BitBoardLeastSignificantBit(relevantBits);
    relevantBits = BitBoardPopBit(relevantBits, s);
    if (index & (1 << i)) relevantBitsSubset = BitBoardSetBit(relevantBitsSubset, s);
  }
  return relevantBitsSubset;
}

static BitBoard getAttackBits(Square s, Type t, Color c) {
  BitBoard relevantBits = getAttacks(s, t, c, EMPTY_BOARD);
  BitBoard edges[NUM_EDGES] = EDGES;
  BitBoard piece = BitBoardSetBit(EMPTY_BOARD, s);
  for (int i = 0; i < NUM_EDGES; i++) {
    if (!((piece & edges[i]) == piece)) {
      relevantBits &= ~edges[i];
    }
  }
  return relevantBits;
}

static BitBoard getMoveBits(Square s, Type t, Color c) {
  if (t != Pawn) return getAttackBits(s, t, c);
  return getMoves(s, t, c, EMPTY_BOARD) | getAttacks(s, t, c, EMPTY_BOARD);
}

static int magicHash(Magic m, BitBoard occupancies) {
  return (int)(((m.relevantBits & occupancies) * m.magicNumber) >> (BOARD_SIZE - m.relevantBitsSize));
}

static Magic getMagic(Square s, Type t, Color c) {

  // account for king and knight later etc

  BitBoard relevantBits = getMoveBits(s, t, c);
  int relevantBitsSize = BitBoardCountBits(relevantBits);
  int relevantBitsPowersetSize = GET_POWERSET_SIZE(relevantBitsSize);

  BitBoard *relevantBitsPowerset = malloc(sizeof(BitBoard) * relevantBitsPowersetSize);
  BitBoard *moves = malloc(sizeof(BitBoard) * relevantBitsPowersetSize);
  if (relevantBitsPowerset == NULL || moves == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < relevantBitsPowersetSize; i++) {
    relevantBitsPowerset[i] = getBitsSubset(i, relevantBits, relevantBitsSize);
    moves[i] = getMoves(s, t, c, relevantBitsPowerset[i]);
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