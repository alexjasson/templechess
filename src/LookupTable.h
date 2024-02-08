#ifndef LOOKUP_TABLE_H
#define LOOKUP_TABLE_H

#define COLOR_SIZE 2
#define TYPE_SIZE 6

#define WHITE_KINGSIDE_CASTLING 0x9000000000000000
#define WHITE_QUEENSIDE_CASTLING 0x1100000000000000
#define BLACK_KINGSIDE_CASTLING 0x0000000000000090
#define BLACK_QUEENSIDE_CASTLING 0x0000000000000011

#define CASTLING_RIGHTS_MASK 0x9100000000000091
#define CASTLING_ATTACK_MASK 0x6C0000000000006C
#define CASTLING_OCCUPANCY_MASK 0x6E0000000000006E

typedef struct lookupTable *LookupTable;

typedef enum { Pawn, King, Knight, Bishop, Rook, Queen } Type;
typedef enum { White, Black, Union } Color;

LookupTable LookupTableNew(void);
void LookupTableFree(LookupTable l);

BitBoard LookupTableGetPawnAttacks(LookupTable l, Square s, Color c);
BitBoard LookupTableGetKnightAttacks(LookupTable l, Square s);
BitBoard LookupTableGetKingAttacks(LookupTable l, Square s);
BitBoard LookupTableGetBishopAttacks(LookupTable l, Square s, BitBoard o);
BitBoard LookupTableGetRookAttacks(LookupTable l, Square s, BitBoard o);
BitBoard LookupTableGetQueenAttacks(LookupTable l, Square s, BitBoard o);

BitBoard LookupTableGetPawnMoves(LookupTable l, Square s, Color c, BitBoard o);
BitBoard LookupTableGetCastling(LookupTable l, Color c, BitBoard castling, BitBoard occupancies, BitBoard attacked);
BitBoard LookupTableGetEnPassant(LookupTable l, Square s, Color c, Square EnPassant);

BitBoard LookupTableGetSquaresBetween(LookupTable l, Square s1, Square s2);
BitBoard LookupTableGetLineOfSight(LookupTable l, Square s1, Square s2);
BitBoard LookupTableGetRank(LookupTable l, Square s);

#endif