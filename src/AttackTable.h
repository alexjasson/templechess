#ifndef ATTACK_TABLE_H
#define ATTACK_TABLE_H

typedef struct attackTable *AttackTable;

typedef struct {
    int x, y;
} Vector2D;

typedef enum { Pawn, Knight, Bishop, King, Rook, Queen } Piece;
typedef enum { White, Black } Color;

AttackTable AttackTableNew(void);
void AttackTableFree(AttackTable a);
BitBoard AttackTableGetPieceAttacks(AttackTable a, Piece p, Color c, Square s);

#endif