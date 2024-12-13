#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "BitBoard.h"
#include "LookupTable.h"

#define TRUE 1
#define FALSE 0
#define IS_DIAGONAL(d) (d % 2 == 1)
#define POWERSET_SIZE(n) (1 << n)
#define DIMENSION_SIZE 2
#define BISHOP_ATTACKS_POWERSET 512
#define ROOK_ATTACKS_POWERSET 4096
#define MAGIC_NUMBERS "data/magicNumbers.out"

typedef enum
{
  North,
  Northeast,
  East,
  Southeast,
  South,
  Southwest,
  West,
  Northwest
} Direction;

typedef struct
{
  BitBoard bits;
  int bitShift;
  uint64_t magicNumber;
} Magic;

struct lookupTable
{
  BitBoard knightAttacks[BOARD_SIZE][DIMENSION_SIZE];
  BitBoard kingAttacks[BOARD_SIZE][DIMENSION_SIZE];
  BitBoard bishopAttacks[BOARD_SIZE][BISHOP_ATTACKS_POWERSET][DIMENSION_SIZE];
  BitBoard rookAttacks[BOARD_SIZE][ROOK_ATTACKS_POWERSET][DIMENSION_SIZE];
  Magic bishopMagics[BOARD_SIZE]; // Used for bishop attacks
  Magic rookMagics[BOARD_SIZE];   // Used for rook attacks

  BitBoard squaresBetween[BOARD_SIZE][BOARD_SIZE]; // Squares Between exclusive
  BitBoard lineOfSight[BOARD_SIZE][BOARD_SIZE];    // All squares of a rank/file/diagonal/antidiagonal
};

static BitBoard getMove(Square s, Type t, Direction d, int steps);
static BitBoard getAttacks(Square s, Type t, BitBoard occupancies);

static BitBoard getRelevantBits(Square s, Type t);
static BitBoard getBitsSubset(int index, BitBoard bits);
static Magic getMagic(Square s, Type t, FILE *fp);
static int magicHash(Magic m, BitBoard occupancies);
static uint64_t getRandomU64();
static uint32_t xorshift();

static BitBoard getSquaresBetween(LookupTable l, Square s1, Square s2);
static BitBoard getLineOfSight(LookupTable l, Square s1, Square s2);
static void initializeLookupTable(LookupTable l);

LookupTable LookupTableNew(void)
{
  LookupTable l = malloc(sizeof(struct lookupTable));
  if (l == NULL)
  {
    fprintf(stderr, "Insufficient memory!\n");
    exit(EXIT_FAILURE);
  }

  initializeLookupTable(l);

  return l;
}

void initializeLookupTable(LookupTable l)
{
  FILE *fp = fopen(MAGIC_NUMBERS, "a+");
  if (fp == NULL)
  {
    fprintf(stderr, "Failed to open '%s' for reading/writing\n", MAGIC_NUMBERS);
    exit(EXIT_FAILURE);
  }

  for (Square s = 0; s < BOARD_SIZE; s++)
  {
    // Fill both 1d and 2d knight attack tables
    BitBoard attacks = getAttacks(s, Knight, EMPTY_BOARD);
    l->knightAttacks[s][0] = attacks;
    while (attacks)
    {
      Square s1 = BitBoardPopLSB(&attacks);
      l->knightAttacks[s][1] |= getAttacks(s1, Knight, EMPTY_BOARD);
    }

    // Fill both 1d and 2d king attack tables
    attacks = getAttacks(s, King, EMPTY_BOARD);
    l->kingAttacks[s][0] = attacks;
    while (attacks)
    {
      Square s1 = BitBoardPopLSB(&attacks);
      l->kingAttacks[s][1] |= getAttacks(s1, King, EMPTY_BOARD);
    }

    // Fill bishop and rook attack tables
    l->bishopMagics[s] = getMagic(s, Bishop, fp);
    for (int i = 0; i < BISHOP_ATTACKS_POWERSET; i++)
    {
      BitBoard occupancies = getBitsSubset(i, l->bishopMagics[s].bits);
      int index = magicHash(l->bishopMagics[s], occupancies);
      attacks = getAttacks(s, Bishop, occupancies);
      l->bishopAttacks[s][index][0] = attacks;
      while (attacks)
      {
        Square s1 = BitBoardPopLSB(&attacks);
        l->bishopAttacks[s][index][1] |= getAttacks(s1, Bishop, occupancies);
      }
    }

    l->rookMagics[s] = getMagic(s, Rook, fp);
    for (int i = 0; i < ROOK_ATTACKS_POWERSET; i++)
    {
      BitBoard occupancies = getBitsSubset(i, l->rookMagics[s].bits);
      int index = magicHash(l->rookMagics[s], occupancies);
      attacks = getAttacks(s, Rook, occupancies);
      l->rookAttacks[s][index][0] = attacks;
      while (attacks)
      {
        Square s1 = BitBoardPopLSB(&attacks);
        l->rookAttacks[s][index][1] |= getAttacks(s1, Rook, occupancies);
      }
    }
  }
  fclose(fp);

  // Helper tables
  for (Square s1 = 0; s1 < BOARD_SIZE; s1++)
  {
    for (Square s2 = 0; s2 < BOARD_SIZE; s2++)
    {
      l->squaresBetween[s1][s2] = getSquaresBetween(l, s1, s2);
      l->lineOfSight[s1][s2] = getLineOfSight(l, s1, s2);
    }
  }
}

void LookupTableFree(LookupTable l)
{
  free(l);
}

BitBoard LookupTableAttacks(LookupTable l, Square s, Type t, BitBoard occupancies)
{
  switch (t)
  {
  case Knight:
    return l->knightAttacks[s][0];
  case King:
    return l->kingAttacks[s][0];
  case Bishop:
    return l->bishopAttacks[s][magicHash(l->bishopMagics[s], occupancies)][0];
  case Rook:
    return l->rookAttacks[s][magicHash(l->rookMagics[s], occupancies)][0];
  case Queen:
    return l->bishopAttacks[s][magicHash(l->bishopMagics[s], occupancies)][0] |
           l->rookAttacks[s][magicHash(l->rookMagics[s], occupancies)][0];
  default:
    printf("Piece: %d\n", t); // "Invalid piece type\n
    fprintf(stderr, "Invalid piece type\n");
    exit(EXIT_FAILURE);
  }
}

BitBoard LookupTableAttacks2D(LookupTable l, Square s, Type t, BitBoard occupancies)
{
  switch (t)
  {
  case Knight:
    return l->knightAttacks[s][1];
  case King:
    return l->kingAttacks[s][1];
  case Bishop:
    return l->bishopAttacks[s][magicHash(l->bishopMagics[s], occupancies)][1];
  case Rook:
    return l->rookAttacks[s][magicHash(l->rookMagics[s], occupancies)][1];
  case Queen:
    return l->bishopAttacks[s][magicHash(l->bishopMagics[s], occupancies)][1] |
           l->rookAttacks[s][magicHash(l->rookMagics[s], occupancies)][1];
  default:
    fprintf(stderr, "Invalid piece type\n");
    exit(EXIT_FAILURE);
  }
}

BitBoard LookupTableGetSquaresBetween(LookupTable l, Square s1, Square s2)
{
  return l->squaresBetween[s1][s2];
}

BitBoard LookupTableGetLineOfSight(LookupTable l, Square s1, Square s2)
{
  return l->lineOfSight[s1][s2];
}

static BitBoard getAttacks(Square s, Type t, BitBoard occupancies)
{
  BitBoard attacks = EMPTY_BOARD;

  // Loop through attacks, if attack does not meet criteria for piece then break/continue
  for (Direction d = North; d <= Northwest; d++)
  {
    for (int steps = 1; steps < EDGE_SIZE; steps++)
    {
      if ((t == Bishop && !IS_DIAGONAL(d)) || (t == Rook && IS_DIAGONAL(d)))
        continue;
      else if (t == Knight && IS_DIAGONAL(d))
        break;
      else if (t <= Knight && steps > 1)
        break;

      BitBoard attack = getMove(s, t, d, steps);
      attacks |= attack;
      if (attack & occupancies)
        break;
    }
  }
  return attacks;
}

static BitBoard getRelevantBits(Square s, Type t)
{
  BitBoard relevantBits = getAttacks(s, t, EMPTY_BOARD);
  BitBoard piece = BitBoardSetBit(EMPTY_BOARD, s);
  if (piece & ~NORTH_EDGE)
    relevantBits &= ~NORTH_EDGE;
  if (piece & ~SOUTH_EDGE)
    relevantBits &= ~SOUTH_EDGE;
  if (piece & ~EAST_EDGE)
    relevantBits &= ~EAST_EDGE;
  if (piece & ~WEST_EDGE)
    relevantBits &= ~WEST_EDGE;

  return relevantBits;
}

// Return a bitboard that represents a square that a piece is attacking
static BitBoard getMove(Square s, Type t, Direction d, int steps)
{
  int rankOffset = (d >= Southeast && d <= Southwest) ? steps : (d <= Northeast || d == Northwest) ? -steps
                                                                                                   : 0;
  int fileOffset = (d >= Northeast && d <= Southeast) ? steps : (d >= Southwest && d <= Northwest) ? -steps
                                                                                                   : 0;
  int rank = BitBoardGetRank(s);
  int file = BitBoardGetFile(s);

  // Check for out-of-bounds conditions
  if ((rank + rankOffset >= EDGE_SIZE || rank + rankOffset < 0) ||
      (file + fileOffset >= EDGE_SIZE || file + fileOffset < 0))
  {
    return EMPTY_BOARD;
  }

  if (!(t == Knight && steps == 1))
  {
    return BitBoardSetBit(EMPTY_BOARD, s + EDGE_SIZE * rankOffset + fileOffset);
  }

  // Handle case where the knight hasn't finished its move
  int offset = (d == North) ? -EDGE_SIZE : (d == South) ? EDGE_SIZE
                                       : (d == East)    ? 1
                                                        : -1;
  Direction d1 = (d == North || d == South) ? East : North;
  Direction d2 = (d == North || d == South) ? West : South;
  return (getMove(s + offset, t, d1, 2) | getMove(s + offset, t, d2, 2));
}

static BitBoard getBitsSubset(int index, BitBoard bits)
{
  int numBits = BitBoardCountBits(bits);
  BitBoard relevantBitsSubset = EMPTY_BOARD;
  for (int i = 0; i < numBits; i++)
  {
    Square s = BitBoardGetLSB(bits);
    bits = BitBoardPopBit(bits, s);
    if (index & (1 << i))
      relevantBitsSubset = BitBoardSetBit(relevantBitsSubset, s);
  }
  return relevantBitsSubset;
}

static int magicHash(Magic m, BitBoard occupancies)
{
  return (int)(((m.bits & occupancies) * m.magicNumber) >> (m.bitShift));
}

// Plain magic bitboards implementation - See https://www.chessprogramming.org/Magic_Bitboards#Plain
static Magic getMagic(Square s, Type t, FILE *fp)
{
  Magic m;
  m.bits = getRelevantBits(s, t);
  m.bitShift = BOARD_SIZE - BitBoardCountBits(m.bits);
  // Attempt to read magic number from file
  int result = fscanf(fp, "%lu", &m.magicNumber);
  if (result == 1)
    return m;

  // Generate magic number if reading failed
  int powersetSize = POWERSET_SIZE((BOARD_SIZE - m.bitShift));
  BitBoard relevantBitsPowerset[powersetSize], attacks[powersetSize], usedAttacks[powersetSize];

  for (int i = 0; i < powersetSize; i++)
  {
    relevantBitsPowerset[i] = getBitsSubset(i, m.bits);
    attacks[i] = getAttacks(s, t, relevantBitsPowerset[i]);
  }

  while (TRUE)
  {
    uint64_t magicNumberCandidate = getRandomU64() & getRandomU64() & getRandomU64();

    for (int j = 0; j < powersetSize; j++)
      usedAttacks[j] = EMPTY_BOARD;
    int collision = FALSE;

    // Test magic index
    for (int j = 0; j < powersetSize; j++)
    {
      m.magicNumber = magicNumberCandidate;
      int index = magicHash(m, relevantBitsPowerset[j]);
      if (usedAttacks[index] == EMPTY_BOARD)
      {
        usedAttacks[index] = attacks[j];
      }
      else if (usedAttacks[index] != attacks[j])
      {
        collision = TRUE;
        break;
      }
    }
    if (!collision)
    {
      fprintf(fp, "%lu\n", m.magicNumber); // Write the new magic number to file
      break;
    }
  }

  return m;
}

// 64-bit PRNG
static uint64_t getRandomU64()
{
  uint64_t u1, u2, u3, u4;

  u1 = (uint64_t)(xorshift()) & 0xFFFF;
  u2 = (uint64_t)(xorshift()) & 0xFFFF;
  u3 = (uint64_t)(xorshift()) & 0xFFFF;
  u4 = (uint64_t)(xorshift()) & 0xFFFF;

  return u1 | (u2 << 16) | (u3 << 32) | (u4 << 48);
}

static uint32_t xorshift()
{
  static uint32_t state = 0;
  static int seeded = FALSE;

  // Seed only once
  if (!seeded)
  {
    state = (uint32_t)time(NULL);
    seeded = TRUE;
  }

  uint32_t x = state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;

  state = x;
  return x;
}

static BitBoard getSquaresBetween(LookupTable l, Square s1, Square s2)
{
  BitBoard pieces = BitBoardSetBit(EMPTY_BOARD, s1) | BitBoardSetBit(EMPTY_BOARD, s2);
  BitBoard squaresBetween = EMPTY_BOARD;
  if (BitBoardGetFile(s1) == BitBoardGetFile(s2) || BitBoardGetRank(s1) == BitBoardGetRank(s2))
  {
    squaresBetween = LookupTableAttacks(l, s1, Rook, pieces) &
                     LookupTableAttacks(l, s2, Rook, pieces);
  }
  else if (BitBoardGetDiagonal(s1) == BitBoardGetDiagonal(s2) ||
           BitBoardGetAntiDiagonal(s1) == BitBoardGetAntiDiagonal(s2))
  {
    squaresBetween = LookupTableAttacks(l, s1, Bishop, pieces) &
                     LookupTableAttacks(l, s2, Bishop, pieces);
  }
  return squaresBetween;
}

static BitBoard getLineOfSight(LookupTable l, Square s1, Square s2)
{
  BitBoard lineOfSight = EMPTY_BOARD;
  if (BitBoardGetFile(s1) == BitBoardGetFile(s2) || BitBoardGetRank(s1) == BitBoardGetRank(s2))
  {
    lineOfSight = (LookupTableAttacks(l, s1, Rook, EMPTY_BOARD) &
                   LookupTableAttacks(l, s2, Rook, EMPTY_BOARD)) |
                  BitBoardSetBit(EMPTY_BOARD, s2);
  }
  else if (BitBoardGetDiagonal(s1) == BitBoardGetDiagonal(s2) ||
           BitBoardGetAntiDiagonal(s1) == BitBoardGetAntiDiagonal(s2))
  {
    lineOfSight = (LookupTableAttacks(l, s1, Bishop, EMPTY_BOARD) &
                   LookupTableAttacks(l, s2, Bishop, EMPTY_BOARD)) |
                  BitBoardSetBit(EMPTY_BOARD, s2);
  }
  return lineOfSight;
}
