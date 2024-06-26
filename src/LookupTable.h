#ifndef LOOKUP_TABLE_H
#define LOOKUP_TABLE_H

typedef struct lookupTable *LookupTable;

typedef enum { Pawn, King, Knight, Bishop, Rook, Queen } Type;
typedef enum { White, Black } Color;

LookupTable LookupTableNew(void);
void LookupTableFree(LookupTable l);
BitBoard LookupTableAttacks(LookupTable l, Square s, Type t, BitBoard o);
BitBoard LookupTableGetSquaresBetween(LookupTable l, Square s1, Square s2);
BitBoard LookupTableGetLineOfSight(LookupTable l, Square s1, Square s2);

#endif