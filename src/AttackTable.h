#ifndef ATTACK_TABLE_H
#define ATTACK_TABLE_H

typedef struct attackTable *AttackTable;
typedef uint64_t U64;
typedef uint32_t U32;

typedef enum { Pawn, Knight, King, Bishop, Rook, Queen } Piece;
typedef enum { White, Black } Color;
/*
 * Let p = number of pieces, c = number of colors, s = number of squares,
 * o = rook powerset size, m = bishop powerset size,  l = number of leaper pieces,
 * t = number of slider pieces
 * Memory allocated = 8pcs + 8pso + 20ps ~ 10.5MB
 * Memory used = 8ls + 8s + 8so + 8sm + 20ts ~ 2.4MB
 */

AttackTable AttackTableNew(void);
void AttackTableFree(AttackTable a);
BitBoard AttackTableGetPieceAttacks(AttackTable a, Piece p, Color c, Square s, BitBoard o);



#endif