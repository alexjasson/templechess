# templechess

TODO:

- Generate an array of moves rather than array of chessboards
- Change pieces[6][2] to pieces[12]
- Change occupancies[3] to occupancies[2]
- Have Move uint16_t and MoveType enum (same implementation as cpw except 0 is empty move and only
                                        castling instead of both queenside/kingside castling encoding)
- No "empty piece/empty square" because we will know if it's a capture from MoveType
- Improve ChatGPT generated code in perft.c
- Fix bug with "./perft 2 '8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -'" (g2g4)

Brainstorming:

IS_CAPTURE

NORMAL
DOUBLE_PAWN_PUSH
CASTLING
EN_PASSANT
KNIGHT_PROMOTION
BISHOP_PROMOTION
ROOK_PROMOTION
QUEEN_PROMOTION

example move encoding

0
1 double push
2 enpassant
3
4
5 castling
6 knight promotion capture
7 knight promotion quiet
8 bishop promotion capture
9 bishop promotion quiet
10 rook promotion capture
11 rook promotion quiet
12 queen promotion capture
13 queen promotion quiet
14 normal capture
15 normal quiet

then IS_CAPTURE and MOVE_TYPE can be determined with bitwise operations ie capture/quiet is just even/odd and 0 still works as empty move
and movetype is just discarding the last bit ( >> 1)

so typedef enum { DoublePush, EnPassant, Castling, knightPromotion, BishopPromotion, RookPromotion, QueenPromotion, Normal } MoveType;

should change getPawnMoves to getPawnPushes

actually just discard occupancies in ChessBoard rep and determine it in legal moves generation

should pieces be sent to LookupTable and then have PieceType and GET_COLOR, GET_PIECE in LookupTable
^this makes sense from OOP perspective, consider it?