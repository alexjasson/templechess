#ifndef LOOKUP_TABLE_H
#define LOOKUP_TABLE_H

#include <limits.h>

#define UNDEFINED UINT_MAX
#define COLOR_SIZE 2
#define TYPE_SIZE 6
#define EDGES { 0xFF00000000000000, 0x00000000000000FF, 0x0101010101010101, 0x8080808080808080 }
#define NUM_EDGES 4
#define RANK_7 0x000000000000FF00
#define RANK_2 0x00FF000000000000

typedef struct lookupTable *LookupTable;

typedef enum { Pawn, Knight, King, Bishop, Rook, Queen } Type;
typedef enum { White, Black, Union } Color;

LookupTable LookupTableNew(void);
void LookupTableFree(LookupTable l);
BitBoard LookupTableGetPieceAttacks(LookupTable l, Square s, Type t, Color c, BitBoard o);

#endif