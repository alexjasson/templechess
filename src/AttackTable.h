#ifndef ATTACK_TABLE_H
#define ATTACK_TABLE_H

typedef struct attackTable *AttackTable;

typedef enum { Pawn, Knight, King, Bishop, Rook, Queen } Piece;
typedef enum { White, Black } Color;

AttackTable AttackTableNew(void);
void AttackTableFree(AttackTable a);
BitBoard AttackTableGetPieceAttacks(AttackTable a, Square s, Piece p, Color c, BitBoard o);



#endif