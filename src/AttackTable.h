#ifndef ATTACK_TABLE_H
#define ATTACK_TABLE_H

typedef struct attackTable *AttackTable;
typedef uint64_t U64;
typedef uint16_t Piece;

typedef enum { Pawn, Knight, King, Bishop, Rook, Queen } Type;
typedef enum { White, Black } Color;

AttackTable AttackTableNew(void);
void AttackTableFree(AttackTable a);
BitBoard AttackTableGetPieceAttacks(AttackTable a, Piece p, BitBoard o);



#endif