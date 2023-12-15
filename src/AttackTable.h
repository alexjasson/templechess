#ifndef ATTACK_TABLE_H
#define ATTACK_TABLE_H

typedef struct attackTable *AttackTable;

typedef struct {
    int rankOffset;
    int fileOffset;
} Move;

AttackTable AttackTableNew(void);

void AttackTableFree(AttackTable a);

BitBoard AttackTableGetWhitePawnAttacks(AttackTable a, Square s);
BitBoard AttackTableGetBlackPawnAttacks(AttackTable a, Square s);
BitBoard AttackTableGetKnightAttacks(AttackTable a, Square s);

#endif