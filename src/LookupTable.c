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
  BitBoard bits;
  int bitShift;
  uint64_t magicNumber;
} HashData;

typedef struct {
  volatile bool stop;
  pthread_mutex_t lock;
  BitBoard *relevantBitsPowerset;
  BitBoard *moves;
  HashData magic;
} ThreadData;

struct lookupTable {

  // add squares between
  BitBoard pawnMoves[BOARD_SIZE][COLOR_SIZE][PAWN_MOVES_POWERSET];
  BitBoard pawnAttacks[BOARD_SIZE][COLOR_SIZE];
  BitBoard knightAttacks[BOARD_SIZE];
  BitBoard kingAttacks[BOARD_SIZE];
  BitBoard bishopAttacks[BOARD_SIZE][BISHOP_ATTACKS_POWERSET];
  BitBoard rookAttacks[BOARD_SIZE][ROOK_ATTACKS_POWERSET];

  BitBoard castling[COLOR_SIZE][CASTLING_POWERSET];
  BitBoard enPassant[BOARD_SIZE][COLOR_SIZE][BOARD_SIZE + 1];

  HashData magics[BOARD_SIZE][MAGICS_SIZE]; // Bishop and rook
  HashData basics[BOARD_SIZE][COLOR_SIZE][BASICS_SIZE]; // Pawn and king
};

static BitBoard getMove(Square s, Type t, Direction d, int steps);
static BitBoard getAttacks(Square s, Type t, Color c, BitBoard occupancies);
static BitBoard getPawnMoves(Square s, Color c, BitBoard occupancies);
static BitBoard getCastling(Color c, BitBoard castling);
static BitBoard getEnPassant(Square s, Color c, Square enPassant);

static BitBoard getRelevantBits(Square s, Type t, Color c);
static BitBoard getBitsSubset(int index, BitBoard bits);
static HashData getHashData(Square s, Type t, Color c);
static void *magicNumberSearch(void *arg);
static int magicHash(HashData h, BitBoard occupancies);
static int basicHash(HashData h, BitBoard occupancies);

static void initalizeMoveTables(LookupTable l);
static void initializeHashData(LookupTable l);

LookupTable LookupTableNew(void) {
  LookupTable l = malloc(sizeof(struct lookupTable));
  if (l == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  initializeHashData(l);
  initalizeMoveTables(l);

  return l;
}

void initalizeMoveTables(LookupTable l) {
  // Pawn moves
  for (Square s = a8; s <= h1; s++) {
    for (Color c = White; c <= Black; c++) {
      l->pawnAttacks[s][c] = getAttacks(s, Pawn, c, EMPTY_BOARD);
      for (int i = 0; i < PAWN_MOVES_POWERSET; i++) {
        BitBoard occupancies = getBitsSubset(i, l->basics[s][c][PAWN_INDEX].bits);
        int index = basicHash(l->basics[s][c][PAWN_INDEX], occupancies);
        l->pawnMoves[s][c][index] = getPawnMoves(s, c, occupancies);
      }
    }
  }

  // Minor/major piece moves
  for (Square s = a8; s < h1; s++) {
    l->knightAttacks[s] = getAttacks(s, Knight, DEFAULT_COLOR, EMPTY_BOARD);
    l->kingAttacks[s] = getAttacks(s, King, DEFAULT_COLOR, EMPTY_BOARD);
    for (int i = 0; i < BISHOP_ATTACKS_POWERSET; i++) {
      BitBoard occupancies = getBitsSubset(i, l->magics[s][BISHOP_INDEX].bits);
      int index = magicHash(l->magics[s][BISHOP_INDEX], occupancies);
      l->bishopAttacks[s][index] = getAttacks(s, Bishop, DEFAULT_COLOR, occupancies);
    }
    for (int i = 0; i < ROOK_ATTACKS_POWERSET; i++) {
      BitBoard occupancies = getBitsSubset(i, l->magics[s][ROOK_INDEX].bits);
      int index = magicHash(l->magics[s][ROOK_INDEX], occupancies);
      l->rookAttacks[s][index] = getAttacks(s, Rook, DEFAULT_COLOR, occupancies);
    }
  }

  // Castling
  for (Color c = White; c <= Black; c++) {
    for (int i = 0; i < CASTLING_POWERSET; i++) {
      BitBoard castling = getBitsSubset(i, l->basics[a1][c][KING_INDEX].bits);
      int index = basicHash(l->basics[a1][c][KING_INDEX], castling);
      l->castling[c][index] = getCastling(c, castling);
    }
  }

  // EnPassant
  for (Square s = a8; s <= h1; s++) {
    for (Color c = White; c <= Black; c++) {
      for (Square enPassant = a8; enPassant <= None; enPassant++) {
        l->enPassant[s][c][enPassant] = getEnPassant(s, c, enPassant);
      }
    }
  }
}

void initializeHashData(LookupTable l) {
  bool emptyFile = isFileEmpty(MAGICS_FILEPATH);
  int fileElementsSize = BOARD_SIZE * MAGICS_SIZE;

  if (!emptyFile) readFromFile(l->magics, sizeof(HashData), fileElementsSize, MAGICS_FILEPATH);
  for (Square s = a8; s <= h1; s++) {
    if (emptyFile) l->magics[s][BISHOP_INDEX] = getHashData(s, Bishop, DEFAULT_COLOR);
    if (emptyFile) l->magics[s][ROOK_INDEX] = getHashData(s, Rook, DEFAULT_COLOR);
  }
  if (emptyFile) writeToFile(l->magics, sizeof(HashData), fileElementsSize, MAGICS_FILEPATH);

  for (Square s = a8; s <= h1; s++) {
    for (Color c = White; c <= Black; c++) {
      l->basics[s][c][PAWN_INDEX] = getHashData(s, Pawn, c);
      l->basics[s][c][KING_INDEX] = getHashData(s, King, c);
    }
  }
}

void LookupTableFree(LookupTable l) {
  free(l);
}

BitBoard LookupTableGetPieceAttacks(LookupTable l, Square s, Type t, Color c, BitBoard occupancies) {
  switch (t) {
    case Pawn: return l->pawnAttacks[s][c];
    case Knight: return l->knightAttacks[s];
    case King: return l->kingAttacks[s];
    case Bishop:
      int index = magicHash(l->magics[s][BISHOP_INDEX], occupancies);
      return l->bishopAttacks[s][index];
    case Rook:
      index = magicHash(l->magics[s][ROOK_INDEX], occupancies);
      return l->rookAttacks[s][index];
    case Queen:
      return LookupTableGetPieceAttacks(l, s, Bishop, c, occupancies) |
             LookupTableGetPieceAttacks(l, s, Rook, c, occupancies);
    default:
      fprintf(stderr, "Invalid piece type!\n");
      exit(EXIT_FAILURE);
  }
}

BitBoard LookupTableGetPawnMoves(LookupTable l, Square s, Color c, BitBoard occupancies) {
  int index = basicHash(l->basics[s][c][PAWN_INDEX], occupancies);
  return l->pawnMoves[s][c][index];
}

BitBoard LookupTableGetCastling(LookupTable l, Color c, BitBoard castling) {
  int index = basicHash(l->basics[a1][c][KING_INDEX], castling);
  return l->castling[c][index];
}

BitBoard LookupTableGetEnPassant(LookupTable l, Square s, Color c, Square enPassant) {
  return l->enPassant[s][c][enPassant];
}

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

static BitBoard getPawnMoves(Square s, Color c, BitBoard occupancies) {
  BitBoard moves = EMPTY_BOARD;
  bool initialRank = (c == White) ? BitBoardGetRank(s) == 6 : BitBoardGetRank(s) == 1;
  for (int steps = 1; steps <= 2; steps++) {
    Direction d = (c == White) ? North : South;
    BitBoard move = getMove(s, Pawn, d, steps);
    bool capture = move & occupancies;
    if (capture) break;
    if (steps == 2 && !initialRank) break;
    moves |= move;
  }
  return moves;
}


static BitBoard getCastling(Color c, BitBoard castling) {
  BitBoard moves = EMPTY_BOARD;
  if (c == White) {
    if ((castling & WHITE_KINGSIDE) == WHITE_KINGSIDE_CASTLING) moves |= BitBoardSetBit(EMPTY_BOARD, g1);
    if ((castling & WHITE_QUEENSIDE) == WHITE_QUEENSIDE_CASTLING) moves |= BitBoardSetBit(EMPTY_BOARD, c1);
  } else {
    if ((castling & BLACK_KINGSIDE) == BLACK_KINGSIDE_CASTLING) moves |= BitBoardSetBit(EMPTY_BOARD, g8);
    if ((castling & BLACK_QUEENSIDE) == BLACK_QUEENSIDE_CASTLING) moves |= BitBoardSetBit(EMPTY_BOARD, c8);
  }
  return moves;
}

// Assume knight or queen will not be passed to this function
static BitBoard getRelevantBits(Square s, Type t, Color c) {
  BitBoard relevantBits = EMPTY_BOARD;
  BitBoard edges[NUM_EDGES] = EDGES;

  if (t == Pawn) {
    Direction d = (c == White) ? North : South;
    relevantBits = getMove(s, Pawn, d, 1);
  } else if (t == King) {
    relevantBits = edges[c];
  } else {
    relevantBits = getAttacks(s, t, c, EMPTY_BOARD);
    BitBoard piece = BitBoardSetBit(EMPTY_BOARD, s);
    for (int i = 0; i < NUM_EDGES; i++) {
      if (!((piece & edges[i]) == piece)) {
        relevantBits &= ~edges[i];
      }
    }
  }

  return relevantBits;
}

// Assume there can only be one en passant move
static BitBoard getEnPassant(Square s, Color c, Square enPassant) {
  BitBoard moves = EMPTY_BOARD;
  BitBoard m1, m2;
  if (c == White) {
    m1 = getMove(s, Pawn, Northeast, 1);
    m2 = getMove(s, Pawn, Northwest, 1);
  } else {
    m1 = getMove(s, Pawn, Southeast, 1);
    m2 = getMove(s, Pawn, Southwest, 1);
  }
  if (m1 & enPassant) moves |= m1;
  if (m2 & enPassant) moves |= m2;
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

static BitBoard getBitsSubset(int index, BitBoard bits) {
  int numBits = BitBoardCountBits(bits);
  BitBoard relevantBitsSubset = EMPTY_BOARD;
  for (int i = 0; i < numBits; i++) {
    Square s = BitBoardLeastSignificantBit(bits);
    bits = BitBoardPopBit(bits, s);
    if (index & (1 << i)) relevantBitsSubset = BitBoardSetBit(relevantBitsSubset, s);
  }
  return relevantBitsSubset;
}

static int magicHash(HashData h, BitBoard occupancies) {
  return (int)(((h.bits & occupancies) * h.magicNumber) >> (h.bitShift));
}

static int basicHash(HashData h, BitBoard occupancies) {
  return (int)((h.bits & occupancies) >> (h.bitShift));
}

static HashData getHashData(Square s, Type t, Color c) {
  HashData h;
  h.bits = getRelevantBits(s, t, c);

  // Return early if piece is pawn or king since they do not use magicHash
  if (t == Pawn) {
    if (c == White) h.bitShift = (s - EDGE_SIZE) % BOARD_SIZE;
    else h.bitShift = (s + EDGE_SIZE) % BOARD_SIZE;
    return h;
  } else if (t == King) {
    if (c == White) h.bitShift = BOARD_SIZE - EDGE_SIZE;
    else h.bitShift = 0;
    return h;
  }

  h.bitShift = BOARD_SIZE - BitBoardCountBits(h.bits);
  int relevantBitsPowersetSize = GET_POWERSET_SIZE((BOARD_SIZE - h.bitShift));

  BitBoard *relevantBitsPowerset = malloc(sizeof(BitBoard) * relevantBitsPowersetSize);
  BitBoard *moves = malloc(sizeof(BitBoard) * relevantBitsPowersetSize);
  if (relevantBitsPowerset == NULL || moves == NULL) {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < relevantBitsPowersetSize; i++) {
    relevantBitsPowerset[i] = getBitsSubset(i, h.bits);
    moves[i] = getAttacks(s, t, c, relevantBitsPowerset[i]);
  }

  ThreadData td = { false, PTHREAD_MUTEX_INITIALIZER, relevantBitsPowerset, moves, h };
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
  int relevantBitsPowersetSize = GET_POWERSET_SIZE((BOARD_SIZE - td->magic.bitShift));

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
      HashData h = { td->magic.bits, td->magic.bitShift, magicNumberCandidate };
      int index = magicHash(h, td->relevantBitsPowerset[j]);
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