#ifndef LOOKUP_TABLE_H
#define LOOKUP_TABLE_H

typedef struct lookupTable *LookupTable;

typedef enum { Pawn, King, Knight, Bishop, Rook, Queen } Type;
typedef enum { White, Black } Color;

LookupTable LookupTableNew(void);
void LookupTableFree(LookupTable l);

BitBoard LookupTableKnightAttacks(LookupTable l, Square s);
BitBoard LookupTableKingAttacks(LookupTable l, Square s);
BitBoard LookupTableBishopAttacks(LookupTable l, Square s, BitBoard o);
BitBoard LookupTableRookAttacks(LookupTable l, Square s, BitBoard o);

BitBoard LookupTableGetSquaresBetween(LookupTable l, Square s1, Square s2);
BitBoard LookupTableGetLineOfSight(LookupTable l, Square s1, Square s2);

#endif