#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#define MAX_DEPTH 16
#define PIECE_SIZE 12

typedef uint8_t BitRank; // index 0 is back rank of black, 7 is back rank of white
typedef uint8_t Piece;

typedef union {
  BitBoard board;
  BitRank rank[EDGE_SIZE];
} BitMap;

typedef struct {
  BitBoard pieces[PIECE_SIZE + 1];
  Piece squares[BOARD_SIZE];

  Color turn;
  int depth; // Start from desired depth and decrement until 0
  BitBoard enPassant[MAX_DEPTH];
  BitMap castling[MAX_DEPTH];
} ChessBoard;

ChessBoard ChessBoardNew(char *fen, int depth); // ChessBoard is stack allocated
void ChessBoardPrint(ChessBoard cb);
void ChessBoardTreeSearch(LookupTable l, ChessBoard cb);
// void ChessBoardAddChildren(ChessBoard parent, ChessBoard *children, LookupTable l);

#endif