#ifndef ATTACK_TABLE_H
#define ATTACK_TABLE_H

typedef struct attackTable *AttackTable;
typedef uint64_t U64;

typedef enum { Pawn, Knight, King, Bishop, Rook, Queen } Piece;
typedef enum { White, Black } Color;

AttackTable AttackTableNew(void);
void AttackTableFree(AttackTable a);
BitBoard AttackTableGetPieceAttacks(AttackTable a, Piece p, Color c, Square s, BitBoard o);

unsigned int generateRandomNumber(void);
U64 randomU64(void);
U64 randomFewbits(void);
int hashFunction(BitBoard key, U64 magicNumber, int occupancySize);
U64 getMagicNumber(Piece p, Square s, int occupancySize);

#endif