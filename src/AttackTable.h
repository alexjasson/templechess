#ifndef ATTACK_TABLE_H
#define ATTACK_TABLE_H

typedef struct attackTable *AttackTable;
typedef uint64_t U64;
typedef uint32_t U32;

typedef enum { Pawn, Knight, King, Bishop, Rook, Queen } Piece;
typedef enum { White, Black } Color;
/*
 * Let p = number of pieces, c = number of colors, s = number of squares,
 * o = occupancy powerset size, l = number of leaper pieces, t = number of slider pieces
 * Memory allocated = 8(p*c*s) + 8(p*s*o) + 20(p*s) + 8 ~ 10.5MB
 */

AttackTable AttackTableNew(void);
void AttackTableFree(AttackTable a);
BitBoard AttackTableGetPieceAttacks(AttackTable a, Piece p, Color c, Square s, BitBoard o);



#endif