#ifndef LOOKUP_TABLE_H
#define LOOKUP_TABLE_H

#define UNDEFINED -1
#define COLOR_SIZE 2
#define TYPE_SIZE 6
#define EDGES { 0xFF00000000000000, 0x00000000000000FF, 0x0101010101010101, 0x8080808080808080 }
#define NUM_EDGES 4

#define PAWN_MOVES_POWERSET 2
#define CASTLING_POWERSET 256
#define BISHOP_ATTACKS_POWERSET 512
#define ROOK_ATTACKS_POWERSET 4096

#define WHITE_KINGSIDE_CASTLING 0x9000000000000000
#define WHITE_QUEENSIDE_CASTLING 0x1100000000000000
#define BLACK_KINGSIDE_CASTLING 0x0000000000000090
#define BLACK_QUEENSIDE_CASTLING 0x0000000000000011

#define WHITE_KINGSIDE 0xF000000000000000
#define WHITE_QUEENSIDE 0x1F00000000000000
#define BLACK_KINGSIDE 0x00000000000000F0
#define BLACK_QUEENSIDE 0x000000000000001F

#define CASTLING_RIGHTS_MASK 0x9100000000000091
#define CASTLING_ATTACK_MASK 0x6C0000000000006C
#define CASTLING_OCCUPANCY_MASK 0x6E0000000000006E

typedef struct lookupTable *LookupTable;

typedef enum { Pawn, King, Knight, Bishop, Rook, Queen } Type;
typedef enum { White, Black, Union } Color;

LookupTable LookupTableNew(void);
void LookupTableFree(LookupTable l);

BitBoard LookupTableGetPieceAttacks(LookupTable l, Square s, Type t, Color c, BitBoard o);
BitBoard LookupTableGetPawnMoves(LookupTable l, Square s, Color c, BitBoard o);
BitBoard LookupTableGetCastling(LookupTable l, Color c, BitBoard castling);
BitBoard LookupTableGetEnPassant(LookupTable l, Square s, Color c, Square EnPassant);

BitBoard LookupTableGetSquaresBetween(LookupTable l, Square s1, Square s2);
BitBoard LookupTableGetLineOfSight(LookupTable l, Square s1, Square s2);


#endif