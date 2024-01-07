#ifndef LOOKUP_TABLE_H
#define LOOKUP_TABLE_H

typedef struct lookupTable *LookupTable;

typedef enum { Pawn, Knight, King, Bishop, Rook, Queen } Piece;
typedef enum { White, Black } Color;

LookupTable LookupTableNew(void);
void LookupTableFree(LookupTable l);
BitBoard LookupTableGetPieceAttacks(LookupTable l, Square s, Piece p, Color c, BitBoard o);

#endif