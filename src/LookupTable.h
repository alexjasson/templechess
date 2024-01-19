#ifndef LOOKUP_TABLE_H
#define LOOKUP_TABLE_H

#include <limits.h>

#define UNDEFINED UINT_MAX
#define COLOR_SIZE 2
#define TYPE_SIZE 6
#define EDGES { 0xFF00000000000000, 0x00000000000000FF, 0x0101010101010101, 0x8080808080808080 }
#define NUM_EDGES 4

#define WHITE_KINGSIDE_CASTLING 0x9000000000000000
#define WHITE_QUEENSIDE_CASTLING 0x1100000000000000
#define BLACK_KINGSIDE_CASTLING 0x0000000000000090
#define BLACK_QUEENSIDE_CASTLING 0x0000000000000011

typedef struct lookupTable *LookupTable;

typedef enum { Pawn, Knight, King, Bishop, Rook, Queen } Type; // new Order? Pawn, Bishop, Rook, Knight, King, Queen
typedef enum { White, Black, Union } Color;

LookupTable LookupTableNew(void);
void LookupTableFree(LookupTable l);
BitBoard LookupTableGetPieceAttacks(LookupTable l, Square s, Type t, Color c, BitBoard o);

#endif