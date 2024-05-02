#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"

#define EMPTY_PIECE 0
#define GET_PIECE(t, c) (((t << 1) | c) + 1)
#define GET_TYPE(p) ((p - 1) >> 1)
#define GET_COLOR(p) ((p - 1) & 1)

#define OUR(t) (cb->pieces[GET_PIECE(t, cb->turn)]) // Bitboard representing our pieces of type t
#define THEIR(t) (cb->pieces[GET_PIECE(t, !cb->turn)]) // Bitboard representing their pieces of type t
#define ALL (~cb->pieces[EMPTY_PIECE]) // Bitboard of all the pieces

#define BOTHSIDE_CASTLING 0b10001001
#define KINGSIDE_CASTLING 0b00001001
#define QUEENSIDE_CASTLING 0b10001000

// Given a square, returns a bitboard representing the rank of that square
#define GET_RANK(s) (SOUTH_EDGE >> (EDGE_SIZE * (EDGE_SIZE - BitBoardGetRank(s) - 1)))
#define PAWN_ATTACKS(b, c) ((c == White) ? BitBoardShiftNW(b) | BitBoardShiftNE(b) : BitBoardShiftSW(b) | BitBoardShiftSE(b))
#define PAWN_ATTACKS_LEFT(b, c) ((c == White) ? BitBoardShiftNW(b) : BitBoardShiftSE(b))
#define PAWN_ATTACKS_RIGHT(b, c) ((c == White) ? BitBoardShiftNE(b) : BitBoardShiftSW(b))
#define SINGLE_PUSH(b, c) ((c == White) ? BitBoardShiftN(b) : BitBoardShiftS(b))

typedef struct {
  BitBoard to;
  BitBoard from;
} Branch;
typedef long (*TraverseFn)(LookupTable, ChessBoard *, Branch);

static Color getColorFromASCII(char asciiColor);
static Piece getPieceFromASCII(char asciiPiece);
static char getASCIIFromPiece(Piece p);

static Piece makeMove(ChessBoard *cb, Square from, Square to, Piece moving);
static void unmakeMove(ChessBoard *cb, Square from, Square to, Piece captured);
static BitMap getAttackedSquares(LookupTable l, ChessBoard *cb, BitBoard them);
static BitBoard getCheckingPieces(LookupTable l, ChessBoard *cb, BitBoard them, BitBoard *pinned);
static long treeSearch(LookupTable l, ChessBoard *cb, TraverseFn traverseFn);

static void printMove(Square from, Square to, long nodes);
static long traverseMoves(LookupTable l, ChessBoard *cb, Branch br);
static long printMoves(LookupTable l, ChessBoard *cb, Branch br);
static long countMoves(LookupTable l, ChessBoard *cb, Branch br);

static Piece movePiece(ChessBoard *cb, Square from, Square to, Piece replacement);

// Assumes FEN and depth is valid
ChessBoard ChessBoardNew(char *fen, int depth) {
  ChessBoard cb;
  memset(&cb, 0, sizeof(ChessBoard));

  // Parse pieces and squares
  for (Square s = a8; s <= h1 && *fen; fen++) {
    if (*fen == '/') continue;

    if (*fen >= '1' && *fen <= '8') {
      for (int numSquares = *fen - '0'; numSquares > 0; numSquares--) {
        cb.pieces[EMPTY_PIECE] |= BitBoardSetBit(EMPTY_BOARD, s);
        cb.squares[s] = EMPTY_PIECE;
        s++;
      }
    } else {
      Piece p = getPieceFromASCII(*fen);
      cb.pieces[p] |= BitBoardSetBit(EMPTY_BOARD, s);
      cb.squares[s] = p;
      s++;
    }
  }
  fen++;

  // Parse turn and depth
  cb.turn = getColorFromASCII(*fen);
  cb.depth = depth;
  fen += 2;

  // Parse castling
  if (*fen == '-') {
    fen++;
  } else {
    while (*fen != ' ') {
      int rank = (*fen == 'K' || *fen == 'Q') ? 7 : 0;
      BitRank flag = (*fen == 'K' || *fen == 'k') ? KINGSIDE_CASTLING : QUEENSIDE_CASTLING;
      cb.castling[depth - 1].rank[rank] |= flag;
      fen++;
    }
    fen++;
  }

  // Parse en passant
  if (*fen != '-') {
    int file = *fen - 'a';
    fen++;
    int rank = EDGE_SIZE - (*fen - '0');
    cb.enPassant[depth - 1] = BitBoardSetBit(EMPTY_BOARD, rank * EDGE_SIZE + file);
  }

  return cb;
}

void ChessBoardPrint(ChessBoard cb) {
  for (int rank = 0; rank < EDGE_SIZE; rank++) {
    for (int file = 0; file < EDGE_SIZE; file++) {
      Square s = rank * EDGE_SIZE + file;
      Piece p = cb.squares[s];
      printf("%c ", getASCIIFromPiece(p));
    }
    printf("%d\n", EDGE_SIZE - rank);
  }
  printf("a b c d e f g h\n\n");
  printf("Color: %d\n", cb.turn);
  printf("Depth: %d\n", cb.depth);
}


// Assumes that asciiPiece is valid
static Piece getPieceFromASCII(char asciiPiece) {
  if (asciiPiece == '-') return EMPTY_PIECE;
  char *pieces = "PKNBRQ";
  Type t = (Type)(strchr(pieces, toupper(asciiPiece)) - pieces);
  Color c = isupper(asciiPiece) ? White : Black;
  return GET_PIECE(t, c);
}

// Assumes that piece is valid
static char getASCIIFromPiece(Piece p) {
  if (p == EMPTY_PIECE) return '-';
  char *pieces = "PKNBRQ";
  char asciiPiece = pieces[GET_TYPE(p)];
  return GET_COLOR(p) == Black ? tolower(asciiPiece) : asciiPiece;
}

static Color getColorFromASCII(char asciiColor) {
  return (asciiColor == 'w') ? White : Black;
}

static long treeSearch(LookupTable l, ChessBoard *cb, TraverseFn traverseFn) {
  // Base cases
  if (cb->depth == 0) return 1;
  // If we are at the node before the leaf nodes and still traversing, start counting moves
  if (cb->depth == 1 && traverseFn == traverseMoves) return treeSearch(l, cb, countMoves);
  long nodes = 0;

  // General purpose BitBoards/Squares
  BitBoard b1, b2;
  Branch br;
  Square s1;

  // Data needed for move generation
  Square ourKing = BitBoardGetLSB(OUR(King));
  BitBoard us, them, pinned, checking;
  BitMap occupancies, attacked;
  int numChecking;
  us = them = pinned = checking = EMPTY_BOARD;

  for (Type t = Pawn; t <= Queen; t++) us |= OUR(t);
  occupancies.board = ALL;
  them = occupancies.board & ~us;
  attacked = getAttackedSquares(l, cb, them);
  checking = getCheckingPieces(l, cb, them, &pinned);
  numChecking = BitBoardCountBits(checking);

  // Traverse king moves
  br.to = LookupTableAttacks(l, ourKing, King, occupancies.board) & ~us & ~attacked.board;
  br.from = OUR(King);
  nodes += traverseFn(l, cb, br);

  if (numChecking > 1) return nodes; // Double check, only king can move

  if (numChecking > 0) { // Single check

    BitBoard checkMask = checking | LookupTableGetSquaresBetween(l, BitBoardGetLSB(checking), ourKing);

    // Traverse non pinned piece moves
    b1 = us & ~(OUR(Pawn) | OUR(King)) & ~pinned;
    while (b1) {
      s1 = BitBoardPopLSB(&b1);
      br.to = LookupTableAttacks(l, s1, GET_TYPE(cb->squares[s1]), occupancies.board) & checkMask;
      br.from = BitBoardSetBit(EMPTY_BOARD, s1);
      nodes += traverseFn(l, cb, br);
    }

    // Traverse non pinned left pawn attacks
    b1 = PAWN_ATTACKS_LEFT(OUR(Pawn) & ~pinned, cb->turn) & checkMask & them;
    if (b1) {
      br.to = b1;
      br.from = PAWN_ATTACKS_LEFT(br.to, !cb->turn);
      nodes += traverseFn(l, cb, br);
    }

    // Traverse non pinned right pawn attacks
    b1 = PAWN_ATTACKS_RIGHT(OUR(Pawn) & ~pinned, cb->turn) & checkMask & them;
    if (b1) {
      br.to = b1;
      br.from = PAWN_ATTACKS_RIGHT(br.to, !cb->turn);
      nodes += traverseFn(l, cb, br);
    }

    // Traverse non pinned single pawn pushes
    b1 = SINGLE_PUSH(OUR(Pawn) & ~pinned, cb->turn) & checkMask & ~ALL;
    if (b1) {
      br.to = b1;
      br.from = SINGLE_PUSH(br.to, !cb->turn);
      nodes += traverseFn(l, cb, br);
    }

    return nodes;
  }

  // No check

  // Traverse non pinned piece moves
  b1 = us & ~(OUR(Pawn) | OUR(King)) & ~pinned;
  while (b1) {
    s1 = BitBoardPopLSB(&b1);
    br.to = LookupTableAttacks(l, s1, GET_TYPE(cb->squares[s1]), occupancies.board) & ~us;
    br.from = BitBoardSetBit(EMPTY_BOARD, s1);
    nodes += traverseFn(l, cb, br);
  }

  // Traverse non pinned left pawn attacks
  b1 = PAWN_ATTACKS_LEFT(OUR(Pawn) & ~pinned, cb->turn) & them;
  if (b1) {
    br.to = b1;
    br.from = PAWN_ATTACKS_LEFT(br.to, !cb->turn);
    nodes += traverseFn(l, cb, br);
  }

  // Traverse non pinned right pawn attacks
  b1 = PAWN_ATTACKS_RIGHT(OUR(Pawn) & ~pinned, cb->turn) & them;
  if (b1) {
    br.to = b1;
    br.from = PAWN_ATTACKS_RIGHT(br.to, !cb->turn);
    nodes += traverseFn(l, cb, br);
  }

  // Traverse non pinned single pawn pushes
  b1 = SINGLE_PUSH(OUR(Pawn) & ~pinned, cb->turn) & ~ALL;
  if (b1) {
    br.to = b1;
    br.from = SINGLE_PUSH(br.to, !cb->turn);
    nodes += traverseFn(l, cb, br);
  }

  // Traverse pinned piece moves
  b1 = (OUR(Bishop) | OUR(Rook)| OUR(Queen)) & pinned;
  while (b1) {
    s1 = BitBoardPopLSB(&b1);
    // Remove moves that are not on the pin line
    br.to = LookupTableAttacks(l, s1, GET_TYPE(cb->squares[s1]), occupancies.board)
          & LookupTableGetLineOfSight(l, ourKing, s1);
    br.from = BitBoardSetBit(EMPTY_BOARD, s1);
    nodes += traverseFn(l, cb, br);
  }

  // Traverse pinned pawn moves
  // Since there are not going to be many pinned pawns, we do a one-to-many mapping
  // Note: Pinned pawns can't do en passant
  b1 = OUR(Pawn) & pinned;
  while (b1) {
    s1 = BitBoardPopLSB(&b1);
    b2 = BitBoardSetBit(EMPTY_BOARD, s1);
    br.to = PAWN_ATTACKS(b2, cb->turn) & them;
    br.to |= SINGLE_PUSH(b2, cb->turn) & ~ALL;
    br.to &= LookupTableGetLineOfSight(l, ourKing, s1);
    br.from = b2;
    nodes += traverseFn(l, cb, br);
  }

  return nodes;
}

long traverseMoves(LookupTable l, ChessBoard *cb, Branch br) {
  Square from = BitBoardGetLSB(br.from), to;
  Piece moving = cb->squares[from], captured;
  long nodes = 0;
  int oneToOne = BitBoardCountBits(br.from) - 1; // If non zero, then one-to-one mapping

  while (br.to) {
    to = BitBoardPopLSB(&br.to);
    if (oneToOne) from = BitBoardPopLSB(&br.from);
    captured = makeMove(cb, from, to, moving);
    nodes += treeSearch(l, cb, traverseMoves); // Continue traversing
    unmakeMove(cb, from, to, captured);
  }

  return nodes;
}

long printMoves(LookupTable l, ChessBoard *cb, Branch br) {
  Square from = BitBoardGetLSB(br.from), to;
  Piece moving = cb->squares[from], captured;
  long nodes = 0, subTree;
  int oneToOne = BitBoardCountBits(br.from) - 1; // If non zero, then one-to-one mapping

  while (br.to) {
    to = BitBoardPopLSB(&br.to);
    if (oneToOne) from = BitBoardPopLSB(&br.from);
    captured = makeMove(cb, from, to, moving);
    subTree = treeSearch(l, cb, traverseMoves); // Continue traversing
    nodes += subTree;
    printMove(from, to, subTree);
    unmakeMove(cb, from, to, captured);
  }

  return nodes;
}

long countMoves(LookupTable l, ChessBoard *cb, Branch br) {
  (void) l;
  (void) cb;
  return BitBoardCountBits(br.to);
}

void ChessBoardTreeSearch(LookupTable l, ChessBoard cb) {
  long nodes = treeSearch(l, &cb, printMoves);
  printf("\nNodes: %ld\n", nodes);
}

inline static Piece makeMove(ChessBoard *cb, Square from, Square to, Piece moving) {
  Piece captured = movePiece(cb, from, to, EMPTY_PIECE);
  (void) moving;

  cb->turn = !cb->turn;
  cb->depth--;
  cb->enPassant[cb->depth] = EMPTY_BOARD;
  cb->castling[cb->depth].board = cb->castling[cb->depth + 1].board ^ (b1 | b2);

  return captured;
}

inline static void unmakeMove(ChessBoard *cb, Square from, Square to, Piece captured) {
  movePiece(cb, to, from, captured);

  cb->turn = !cb->turn;
  cb->depth++;
}

// The replacement piece replaces the piece that has moved
Piece movePiece(ChessBoard *cb, Square from, Square to, Piece replacement) {
  BitBoard b1 = BitBoardSetBit(EMPTY_BOARD, from);
  BitBoard b2 = BitBoardSetBit(EMPTY_BOARD, to);
  Piece captured = cb->squares[to];
  Piece moving = cb->squares[from];

  cb->squares[to] = moving;
  cb->squares[from] = replacement;
  cb->pieces[moving] ^= (b1 | b2);
  cb->pieces[captured] &= ~b2;
  cb->pieces[replacement] |= b1;

  return captured;
}

void printMove(Square from, Square to, long nodes) {
  printf("%c%d%c%d: %ld\n", 'a' + (from % EDGE_SIZE), EDGE_SIZE - (from / EDGE_SIZE), 'a' + (to % EDGE_SIZE), EDGE_SIZE - (to / EDGE_SIZE), nodes);
}

// Return the checking pieces and simultaneously update the pinned pieces bitboard
BitBoard getCheckingPieces(LookupTable l, ChessBoard *cb, BitBoard them, BitBoard *pinned) {
  Square ourKing = BitBoardGetLSB(OUR(King));
  BitBoard checking = (PAWN_ATTACKS(OUR(King), cb->turn) & THEIR(Pawn)) |
                      (LookupTableAttacks(l, ourKing, Knight, EMPTY_BOARD) & THEIR(Knight));
  BitBoard candidates = (LookupTableAttacks(l, ourKing, Bishop, them) & (THEIR(Bishop) | THEIR(Queen))) |
                        (LookupTableAttacks(l, ourKing, Rook, them) & (THEIR(Rook) | THEIR(Queen)));

  BitBoard b;
  Square s;
  while (candidates) {
    s = BitBoardPopLSB(&candidates);
    b = LookupTableGetSquaresBetween(l, ourKing, s) & ALL & ~them;
    if (b == EMPTY_BOARD) checking |= BitBoardSetBit(EMPTY_BOARD, s);
    else if ((b & (b - 1)) == EMPTY_BOARD) *pinned |= b;
  }

  return checking;
}

// Return the squares attacked by the enemy
static BitMap getAttackedSquares(LookupTable l, ChessBoard *cb, BitBoard them) {
  BitMap attacked;
  BitBoard b;
  BitBoard occupancies = ALL & ~OUR(King);

  attacked.board = PAWN_ATTACKS(THEIR(Pawn), !cb->turn);
  b = them & ~THEIR(Pawn);
  while (b) attacked.board |= LookupTableAttacks(l, BitBoardPopLSB(&b), GET_TYPE(cb->squares[BitBoardGetLSB(b)]), occupancies);

  return attacked;
}