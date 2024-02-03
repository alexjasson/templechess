#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ChessBoard.h"

#define PIECE_SIZE 12
#define GET_PIECE(t, c) ((t << 1) | c)
#define GET_TYPE(p) (p >> 1)
#define GET_COLOR(p) (p & 1)

#define MAX_CHILDREN 256

static Color getColorFromASCII(char asciiColor);
static Piece getPieceFromASCII(char asciiPiece);
static char getASCIIFromPiece(Piece p);
static ChessBoard newChessBoard(void);
static BitBoard getAttackedSquares(ChessBoard cb, LookupTable l);
static ChessBoard makeMove(ChessBoard cb, Type t, Square from, Square to);
static BitBoard getPawnAttacksSet(BitBoard p, Color c);

static ChessBoard newChessBoard(void) {
  ChessBoard cb;
  memset(&cb, 0, sizeof(ChessBoard));
  return cb;
}

// Assumes FEN is valid
ChessBoard ChessBoardFromFEN(char *fen) {
  ChessBoard cb = newChessBoard();

  // Parse pieces and determine occupancies
  Square s = a8;
  while (s <= h1 && *fen) {
    if (*fen == '/') {
      fen++;
    } else if (*fen >= '1' && *fen <= '8') {
      int numSquares = *fen - '0';
      while (numSquares--) {
        cb.squares[s] = EMPTY_SQUARE;
        s++;
      }
      fen++;
    } else {
      Piece p = getPieceFromASCII(*fen);
      Type t = GET_TYPE(p); Color c = GET_COLOR(p);
      cb.pieces[t][c] |= BitBoardSetBit(EMPTY_BOARD, s);
      cb.occupancies[c] |= cb.pieces[t][c];
      cb.occupancies[Union] |= cb.pieces[t][c];
      cb.squares[s] = p;
      s++;
      fen++;
    }
  }

  fen++;

  // Parse turn
  cb.turn = getColorFromASCII(*fen);

  fen++;
  fen++;

  // Parse castling
  if (*fen != '-') {
    while (*fen != ' ') {
      switch (*fen) {
        case 'K': cb.castling |= WHITE_KINGSIDE_CASTLING; break;
        case 'Q': cb.castling |= WHITE_QUEENSIDE_CASTLING; break;
        case 'k': cb.castling |= BLACK_KINGSIDE_CASTLING; break;
        case 'q': cb.castling |= BLACK_QUEENSIDE_CASTLING; break;
      }
      fen++;
    }
  } else {
    fen++;
  }

  fen++;

  // Parse en passant
  if (*fen != '-') {
    int file = *fen - 'a';
    fen++;
    int rank = *fen - '0';
    cb.enPassant = rank * EDGE_SIZE + file;
  } else {
    cb.enPassant = None;
  }

  fen++;
  fen++;

  // Parse half move clock
  cb.halfMoveClock = atoi(fen);

  fen++;
  fen++;

  // Parse move number
  cb.moveNumber = atoi(fen);

  return cb;
}

void ChessBoardPrint(ChessBoard cb) {
  for (int rank = 0; rank < EDGE_SIZE; rank++) {
    for (int file = 0; file < EDGE_SIZE; file++) {
      Square s = rank * EDGE_SIZE + file;
      for (Piece p = 0; p < PIECE_SIZE; p++) {
        if (BitBoardGetBit(cb.pieces[GET_TYPE(p)][GET_COLOR(p)], s)) {
          printf("%c ", getASCIIFromPiece(p));
          goto nextSquare;
        }
      }
      printf("- ");
      nextSquare: continue;
    }
    printf("%d \n", EDGE_SIZE - rank);
  }
  printf("a b c d e f g h\n\n");
}

// Assumes that asciiPiece is valid
static Piece getPieceFromASCII(char asciiPiece) {
  Type t; Color c;
  switch (asciiPiece) {
    case 'P': case 'p': t = Pawn; break;
    case 'N': case 'n': t = Knight; break;
    case 'K': case 'k': t = King; break;
    case 'B': case 'b': t = Bishop; break;
    case 'R': case 'r': t = Rook; break;
    case 'Q': case 'q': t = Queen; break;
  }
  c = (asciiPiece >= 'A' && asciiPiece <= 'Z') ? White : Black;
  return GET_PIECE(t, c);
}

// Assumes that piece is valid
static char getASCIIFromPiece(Piece p) {
  char asciiPiece;
  switch (GET_TYPE(p)) {
    case Pawn: asciiPiece = 'P'; break;
    case Knight: asciiPiece = 'N'; break;
    case King: asciiPiece = 'K'; break;
    case Bishop: asciiPiece = 'B'; break;
    case Rook: asciiPiece = 'R'; break;
    case Queen: asciiPiece = 'Q'; break;
  }
  if (GET_COLOR(p) == Black) asciiPiece = tolower(asciiPiece);
  return asciiPiece;
}

static Color getColorFromASCII(char asciiColor) {
  return (asciiColor == 'w') ? White : Black;
}

// NEEDS WORK!
ChessBoard *ChessBoardGetChildren(ChessBoard cb, LookupTable l) {
  ChessBoard *children = calloc(MAX_CHILDREN, sizeof(ChessBoard));
  int numChildren = 0;

  // BitBoards of attacked squares by enemy, checking pieces by enemy, and our pinned pieces
  BitBoard attacked = EMPTY_BOARD, checking = EMPTY_BOARD, pinned = EMPTY_BOARD;
  Square ourKing = BitBoardGetLSB(cb.pieces[King][cb.turn]); // Our king
  BitBoard edges[COLOR_SIZE] = { NORTH_EDGE, SOUTH_EDGE };

  // General purpose BitBoards/Squares
  BitBoard b1, b2, b3;
  Square s;

  attacked = getAttackedSquares(cb, l);

  // Add legal king moves
  b1 = LookupTableGetKingAttacks(l, ourKing);
  b2 = b1 & ~(cb.occupancies[cb.turn] | attacked);
  while (b2) children[numChildren++] = makeMove(cb, King, ourKing, BitBoardPopLSB(&b2));

  checking = LookupTableGetPawnAttacks(l, ourKing, cb.turn) & cb.pieces[Pawn][!cb.turn];
  checking |= LookupTableGetKnightAttacks(l, ourKing) & cb.pieces[Knight][!cb.turn];

  // Potential checking pieces of enemy
  BitBoard candidates = (LookupTableGetBishopAttacks(l, ourKing, cb.occupancies[!cb.turn]) &
                        (cb.pieces[Bishop][!cb.turn] | cb.pieces[Queen][!cb.turn])) |
                        (LookupTableGetRookAttacks(l, ourKing, cb.occupancies[!cb.turn]) &
                        (cb.pieces[Rook][!cb.turn] | cb.pieces[Queen][!cb.turn]));


  // printf("Candidates\n");
  // BitBoardPrint(candidates);
  // printf("Rook and Queen\n");
  // BitBoardPrint(cb.pieces[Rook][!cb.turn] | cb.pieces[Queen][!cb.turn]);
  // printf("king attacks\n");
  // BitBoardPrint((LookupTableGetRookAttacks(l, s, cb.occupancies[Union])));
  // printf("location of king\n");
  // BitBoardPrint(cb.pieces[King][cb.turn]);

  while (candidates) {
    s = BitBoardPopLSB(&candidates);
    b1 = LookupTableGetSquaresBetween(l, ourKing, s) & cb.occupancies[cb.turn];
    printf("b1\n");
    BitBoardPrint(b1);
    printf("Squares between\n");
    BitBoardPrint(LookupTableGetSquaresBetween(l, ourKing, s));
    if (b1 == EMPTY_BOARD) checking |= BitBoardSetBit(EMPTY_BOARD, s);
    else if ((b1 & (b1 - 1)) == EMPTY_BOARD) pinned |= b1;
  }

  int numChecking = BitBoardCountBits(checking);
  printf("numChecking: %d\n", numChecking);

  printf("pinned\n");
  BitBoardPrint(pinned);
  if (numChecking > 1) return children; // Double check, only king can move



  if (numChecking > 0) {
    // Single check, pieces can either capture checking piece, block it, or move king
    // Pinned pieces cannot move
    // We don't need to consider taking friendly pieces because it is not possible for a friendly piece to be on the checkMask
    BitBoard checkMask = checking | LookupTableGetSquaresBetween(l, BitBoardGetLSB(checking), ourKing);
    printf("check mask\n");
    BitBoardPrint(checkMask);

    // Add legal pawn moves
    b1 = cb.pieces[Pawn][cb.turn] & ~pinned;
    while (b1) {
      s = BitBoardPopLSB(&b1);
      b2 = LookupTableGetPawnAttacks(l, s, cb.turn) & cb.occupancies[!cb.turn];
      b2 |= LookupTableGetPawnMoves(l, s, cb.turn, cb.occupancies[Union]) & ~cb.occupancies[Union];
      b2 |= LookupTableGetEnPassant(l, s, cb.turn, cb.enPassant);
      printf("Pawn attacks\n");
      b2 &= checkMask;
      b3 = b2 & edges[cb.turn]; // b3 contains any moves that result in promotion
      b2 &= ~b3; // b2 contains all non-promotion moves
      BitBoardPrint(b2);
      BitBoardPrint(b3);

      while (b2) children[numChildren++] = makeMove(cb, Pawn, s, BitBoardPopLSB(&b2));
      while (b3) {
        for (Type t = Knight; t <= Queen; t++) children[numChildren++] = makeMove(cb, t, s, BitBoardPopLSB(&b3));
      }
    }

    printf("did we get here?\n");

    // Add legal knight moves
    b1 = cb.pieces[Knight][cb.turn] & ~pinned;
    while (b1) {
      s = BitBoardPopLSB(&b1);
      b2 = LookupTableGetKnightAttacks(l, s) & checkMask;
      while (b2) children[numChildren++] = makeMove(cb, Knight, s, BitBoardPopLSB(&b2));
    }

    // Add legal bishop moves
    b1 = cb.pieces[Bishop][cb.turn] & ~pinned;
    while (b1) {
      s = BitBoardPopLSB(&b1);
      b2 = LookupTableGetBishopAttacks(l, s, cb.occupancies[Union]) & checkMask;
      while (b2) children[numChildren++] = makeMove(cb, Bishop, s, BitBoardPopLSB(&b2));
    }

    // Add legal rook moves
    b1 = cb.pieces[Rook][cb.turn] & ~pinned;
    while (b1) {
      s = BitBoardPopLSB(&b1);
      b2 = LookupTableGetRookAttacks(l, s, cb.occupancies[Union]) & checkMask;
      while (b2) children[numChildren++] = makeMove(cb, Rook, s, BitBoardPopLSB(&b2));
    }

    // Add legal queen moves
    b1 = cb.pieces[Queen][cb.turn] & ~pinned;
    while (b1) {
      s = BitBoardPopLSB(&b1);
      b2 = LookupTableGetQueenAttacks(l, s, cb.occupancies[Union]) & checkMask;
      while (b2) children[numChildren++] = makeMove(cb, Queen, s, BitBoardPopLSB(&b2));
    }

    return children;
  }

  // No check, pinned pieces can only move along pin line, other pieces can move freely
  // If a knight is pinned, it cannot move

  // TODO:

  return children;
}

// Note that the type of piece is meant to represent the piece that is on the
// square that was moved to, not the piece that was moved. It is meant to be used
// in the case of pawn promotion.
static ChessBoard makeMove(ChessBoard cb, Type t, Square from, Square to) {
  int offset = abs((int)(from - to));
  Piece captured = cb.squares[to]; // Note: piece may be empty square
  Piece moving = cb.squares[from];

  // Remove piece from from square
  cb.pieces[GET_TYPE(moving)][GET_COLOR(moving)] = BitBoardPopBit(cb.pieces[GET_TYPE(moving)][GET_COLOR(moving)], from);
  // Put piece on to square
  cb.pieces[t][cb.turn] = BitBoardSetBit(cb.pieces[t][cb.turn], to);
  // Remove piece from to square if it exists
  if (captured != EMPTY_SQUARE) cb.pieces[GET_TYPE(captured)][!cb.turn] = BitBoardPopBit(cb.pieces[GET_TYPE(captured)][!cb.turn], to);
  cb.squares[from] = EMPTY_SQUARE;
  cb.squares[to] = GET_PIECE(t, cb.turn);

  // Determine occupancies
  cb.occupancies[cb.turn] = BitBoardSetBit(BitBoardPopBit(cb.occupancies[cb.turn], from), to);
  cb.occupancies[!cb.turn] = BitBoardPopBit(cb.occupancies[!cb.turn], to);
  cb.occupancies[Union] = BitBoardSetBit(BitBoardPopBit(cb.occupancies[Union], from), to);

  // Handle enpassant move
  if (t == Pawn && offset == 1) makeMove(cb, Pawn, to, cb.enPassant);

  // Update castling bits
  cb.castling ^= cb.castling & BitBoardSetBit(EMPTY_BOARD, from);

  // Update en passant square
  cb.enPassant = (t == Pawn && offset > EDGE_SIZE + 1) ? (from + to) / 2 : None;

  // if (t == Pawn && offset == 1) {
  //   // Handle enPassant - make move again
  // } else if (t == King && offset == 2) {
  //   // Handle castling - make move again
  // }

  // Update half move clock
  if (t == Pawn || captured != EMPTY_SQUARE) {
    cb.halfMoveClock = 0;
} else {
    cb.halfMoveClock++;
}

  // Update move number
  if (cb.turn == Black) cb.moveNumber++;

  // Update color
  cb.turn = !cb.turn;
  return cb;
}

// Faster to calculate pawn attacks set OTF than looking up all individual pawn attacks
static BitBoard getPawnAttacksSet(BitBoard p, Color c) {
  return (c == White) ? BitBoardShiftNorthwest(p) | BitBoardShiftNortheast(p) : BitBoardShiftSouthwest(p) | BitBoardShiftSoutheast(p);
}

static BitBoard getAttackedSquares(ChessBoard cb, LookupTable l) {
  BitBoard attackedSquares;
  BitBoard b1, b2;

  // Pawn attacks
  b1 = cb.pieces[Pawn][!cb.turn]; // Enemy pawns
  attackedSquares = getPawnAttacksSet(b1, !cb.turn);

  // King attacks
  b1 = cb.pieces[King][!cb.turn]; // Enemy king
  attackedSquares |= LookupTableGetKingAttacks(l, BitBoardGetLSB(b1));

  // Knight attacks
  b1 = cb.pieces[Knight][!cb.turn]; // Enemy knights
  while (b1) attackedSquares |= LookupTableGetKnightAttacks(l, BitBoardPopLSB(&b1));

  // Diagonal attacks
  b1 = cb.pieces[Bishop][!cb.turn] | cb.pieces[Queen][!cb.turn]; // Diagonal enemy pieces
  b2 = cb.occupancies[Union] & ~cb.pieces[King][cb.turn]; // All pieces except our king
  while (b1) attackedSquares |= LookupTableGetBishopAttacks(l, BitBoardPopLSB(&b1), b2);

  // Orthogonal attacks
  b1 = cb.pieces[Rook][!cb.turn] | cb.pieces[Queen][!cb.turn]; // Orthogonal enemy pieces
  b2 = cb.occupancies[Union] & ~cb.pieces[King][cb.turn]; // All pieces except our king
  while (b1) attackedSquares |= LookupTableGetRookAttacks(l, BitBoardPopLSB(&b1), b2);

  return attackedSquares;
}