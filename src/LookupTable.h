#ifndef LOOKUP_TABLE_H
#define LOOKUP_TABLE_H

#define COLOR_SIZE 2
#define TYPE_SIZE 6

typedef struct lookupTable *LookupTable;

typedef enum { Pawn, Knight, King, Bishop, Rook, Queen } Type;
typedef enum { White, Black, Union } Color;

LookupTable LookupTableNew(void);
void LookupTableFree(LookupTable l);
BitBoard LookupTableGetPieceAttacks(LookupTable l, Square s, Type t, Color c, BitBoard o);

#endif