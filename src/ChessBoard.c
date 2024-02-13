#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ChessBoard.h"

#define PIECE_SIZE 12
#define GET_PIECE(t, c) ((t << 1) | c)
#define GET_TYPE(p) (p >> 1)
#define GET_COLOR(p) (p & 1)

static Color getColorFromASCII(char asciiColor);
static Piece getPieceFromASCII(char asciiPiece);
static char getASCIIFromPiece(Piece p);
static ChessBoard newChessBoard(void);
static ChessBoard makeMove(ChessBoard cb, Type t, Square from, Square to);
static BitBoard getAttackedSquares(ChessBoard cb, LookupTable l);
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
        case 'K': cb.castling |= CASTLING & KINGSIDE & SOUTH_EDGE; break;
        case 'Q': cb.castling |= CASTLING & QUEENSIDE & SOUTH_EDGE; break;
        case 'k': cb.castling |= CASTLING & KINGSIDE & NORTH_EDGE; break;
        case 'q': cb.castling |= CASTLING & QUEENSIDE & NORTH_EDGE; break;
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
    int rank = EDGE_SIZE - (*fen - '0');
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
    default: fprintf(stderr, "Invalid piece!\n"); exit(EXIT_FAILURE);
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

void ChessBoardAddChildren(ChessBoard parent, ChessBoard *children, LookupTable l) {
  int numChildren = 0;

  // BitBoards of attacked squares by enemy, checking pieces by enemy, and our pinned pieces
  BitBoard attacked = EMPTY_BOARD, checking = EMPTY_BOARD, pinned = EMPTY_BOARD;
  Square ourKing = BitBoardGetLSB(parent.pieces[King][parent.turn]);
  BitBoard promotionRank = (parent.turn == White) ? NORTH_EDGE : SOUTH_EDGE;

  // General purpose BitBoards/Squares
  BitBoard b1, b2, b3;
  Square s;

  attacked = getAttackedSquares(parent, l);

  // Add legal king moves
  b1 = LookupTableGetKingAttacks(l, ourKing);
  b2 = b1 & ~(parent.occupancies[parent.turn] | attacked);
  while (b2) children[numChildren++] = makeMove(parent, King, ourKing, BitBoardPopLSB(&b2));

  // Pawn and knight checking pieces of enemy
  checking = LookupTableGetPawnAttacks(l, ourKing, parent.turn) & parent.pieces[Pawn][!parent.turn];
  checking |= LookupTableGetKnightAttacks(l, ourKing) & parent.pieces[Knight][!parent.turn];

  // Potential orthogonal/diagonal checking pieces of enemy
  BitBoard candidates = (LookupTableGetBishopAttacks(l, ourKing, parent.occupancies[!parent.turn]) &
                        (parent.pieces[Bishop][!parent.turn] | parent.pieces[Queen][!parent.turn])) |
                        (LookupTableGetRookAttacks(l, ourKing, parent.occupancies[!parent.turn]) &
                        (parent.pieces[Rook][!parent.turn] | parent.pieces[Queen][!parent.turn]));

  while (candidates) {
    s = BitBoardPopLSB(&candidates);
    b1 = LookupTableGetSquaresBetween(l, ourKing, s) & parent.occupancies[parent.turn];
    if (b1 == EMPTY_BOARD) checking |= BitBoardSetBit(EMPTY_BOARD, s);
    else if ((b1 & (b1 - 1)) == EMPTY_BOARD) pinned |= b1;
  }

  int numChecking = BitBoardCountBits(checking);

  if (numChecking > 1) return; // Double check, only king can move

  if (numChecking > 0) {
    // Single check - Pieces can either capture checking piece, block it or king can move, pinned pieces can't move
    // We don't need to consider taking our pieces because it is not possible for our piece to be on the checkMask
    BitBoard checkMask = checking | LookupTableGetSquaresBetween(l, BitBoardGetLSB(checking), ourKing);

    // Add legal pawn moves
    b1 = parent.pieces[Pawn][parent.turn] & ~pinned;
    while (b1) {
      s = BitBoardPopLSB(&b1);
      b2 = LookupTableGetPawnAttacks(l, s, parent.turn) & parent.occupancies[!parent.turn];
      b2 |= LookupTableGetPawnPushes(l, s, parent.turn, parent.occupancies[Union]) & ~parent.occupancies[Union];
      b2 |= LookupTableGetEnPassant(l, s, parent.turn, parent.enPassant);
      b2 &= checkMask;

      b3 = b2 & promotionRank; // b3 contains any moves that result in promotion
      b2 &= ~promotionRank; // b2 contains all non-promotion moves

      while (b2) children[numChildren++] = makeMove(parent, Pawn, s, BitBoardPopLSB(&b2));
      while (b3) {
        Square move = BitBoardPopLSB(&b3);
        for (Type t = Knight; t <= Queen; t++) children[numChildren++] = makeMove(parent, t, s, move);
      }
    }

    // Add legal knight moves
    b1 = parent.pieces[Knight][parent.turn] & ~pinned;
    while (b1) {
      s = BitBoardPopLSB(&b1);
      b2 = LookupTableGetKnightAttacks(l, s) & checkMask;
      while (b2) children[numChildren++] = makeMove(parent, Knight, s, BitBoardPopLSB(&b2));
    }

    // Add legal bishop moves
    b1 = parent.pieces[Bishop][parent.turn] & ~pinned;
    while (b1) {
      s = BitBoardPopLSB(&b1);
      b2 = LookupTableGetBishopAttacks(l, s, parent.occupancies[Union]) & checkMask;
      while (b2) children[numChildren++] = makeMove(parent, Bishop, s, BitBoardPopLSB(&b2));
    }

    // Add legal rook moves
    b1 = parent.pieces[Rook][parent.turn] & ~pinned;
    while (b1) {
      s = BitBoardPopLSB(&b1);
      b2 = LookupTableGetRookAttacks(l, s, parent.occupancies[Union]) & checkMask;
      while (b2) children[numChildren++] = makeMove(parent, Rook, s, BitBoardPopLSB(&b2));
    }

    // Add legal queen moves
    b1 = parent.pieces[Queen][parent.turn] & ~pinned;
    while (b1) {
      s = BitBoardPopLSB(&b1);
      b2 = LookupTableGetQueenAttacks(l, s, parent.occupancies[Union]) & checkMask;
      while (b2) children[numChildren++] = makeMove(parent, Queen, s, BitBoardPopLSB(&b2));
    }

    return;
  }

  // No check, pinned pieces can only move along pin line, other pieces can move freely
  // If a knight is pinned, it cannot move
  // Must consider special case of enpassant exposing the king to check laterally, not
  // for pinned enpassant though
  // Note: A friendly piece can't be on a pin mask so we dont need to check pinned pieces
  // capturing any of our pieces
  BitBoard ourPawns = parent.pieces[Pawn][parent.turn];

  // Add en passant
  if (parent.enPassant != None) {
    b1 = ourPawns & LookupTableGetPawnAttacks(l, parent.enPassant, !parent.turn);
    if (b1) {
      BitBoard pinMask = LookupTableGetLineOfSight(l, ourKing, parent.enPassant);
      b2 = b1 & ~pinned; // Non pinned en passant pawns
      b3 = b1 & pinned & pinMask; // Pinned en passant pawns

      while (b3) {
        Square s2 = BitBoardPopLSB(&b3);
        BitBoard move = LookupTableGetEnPassant(l, s2, parent.turn, parent.enPassant);
        children[numChildren++] = makeMove(parent, Pawn, s2, BitBoardGetLSB(move));
      }
      while (b2) {
        Square s2 = BitBoardPopLSB(&b2);
        BitBoard move = LookupTableGetEnPassant(l, s2, parent.turn, parent.enPassant);
        if (LookupTableGetRookAttacks(l, ourKing, parent.occupancies[Union] & ~(move | b2)) &
            LookupTableGetRank(l, ourKing) & (parent.pieces[Rook][!parent.turn] | parent.pieces[Queen][!parent.turn])) break;
        children[numChildren++] = makeMove(parent, Pawn, s2, BitBoardGetLSB(move));
      }
    }
  }


  // Add castling
  b1 = LookupTableGetCastling(l, parent.turn, parent.castling, parent.occupancies[Union], attacked);
  while (b1) children[numChildren++] = makeMove(parent, King, ourKing, BitBoardPopLSB(&b1));


  // Add pinned pawn moves
  b1 = ourPawns & pinned;
  while (b1) {
    s = BitBoardPopLSB(&b1);
    BitBoard pinMask = LookupTableGetLineOfSight(l, ourKing, s);
    b2 = LookupTableGetPawnAttacks(l, s, parent.turn) & parent.occupancies[!parent.turn];
    b3 = b2 & promotionRank & pinMask; // b3 contains any moves that result in promotion
    b2 |= LookupTableGetPawnPushes(l, s, parent.turn, parent.occupancies[Union]) & ~parent.occupancies[!parent.turn];
    b2 &= ~promotionRank & pinMask; // b2 contains all non-promotion moves

    while (b2) children[numChildren++] = makeMove(parent, Pawn, s, BitBoardPopLSB(&b2));
    while (b3) {
      Square move = BitBoardPopLSB(&b3);
      for (Type t = Knight; t <= Queen; t++) children[numChildren++] = makeMove(parent, t, s, move);
    }
  }

  // Add pinned bishop moves
  b1 = parent.pieces[Bishop][parent.turn] & pinned;
  while (b1) {
    s = BitBoardPopLSB(&b1);
    BitBoard pinMask = LookupTableGetLineOfSight(l, ourKing, s);
    b2 = LookupTableGetBishopAttacks(l, s, parent.occupancies[Union]);
    b2 &= pinMask;

    while (b2) children[numChildren++] = makeMove(parent, Bishop, s, BitBoardPopLSB(&b2));
  }

  // Add pinned rook moves
  b1 = parent.pieces[Rook][parent.turn] & pinned;
  while (b1) {
    s = BitBoardPopLSB(&b1);
    BitBoard pinMask = LookupTableGetLineOfSight(l, ourKing, s);
    b2 = LookupTableGetRookAttacks(l, s, parent.occupancies[Union]);
    b2 &= pinMask;

    while (b2) children[numChildren++] = makeMove(parent, Rook, s, BitBoardPopLSB(&b2));
  }

  // Add pinned queen moves
  b1 = parent.pieces[Queen][parent.turn] & pinned;
  while (b1) {
    s = BitBoardPopLSB(&b1);
    BitBoard pinMask = LookupTableGetLineOfSight(l, ourKing, s);
    b2 = LookupTableGetQueenAttacks(l, s, parent.occupancies[Union]);
    b2 &= pinMask;

    while (b2) children[numChildren++] = makeMove(parent, Queen, s, BitBoardPopLSB(&b2));
  }

  // Add pawn moves
  b1 = ourPawns & ~pinned;
  while (b1) {
    s = BitBoardPopLSB(&b1);
    b2 = LookupTableGetPawnAttacks(l, s, parent.turn) & parent.occupancies[!parent.turn];
    b2 |= LookupTableGetPawnPushes(l, s, parent.turn, parent.occupancies[Union]) & ~parent.occupancies[Union];
    b3 = b2 & promotionRank; // b3 contains any moves that result in promotion
    b2 &= ~promotionRank; // b2 contains all non-promotion moves

    while (b2) children[numChildren++] = makeMove(parent, Pawn, s, BitBoardPopLSB(&b2));
    while (b3) {
      Square move = BitBoardPopLSB(&b3);
      for (Type t = Knight; t <= Queen; t++) children[numChildren++] = makeMove(parent, t, s, move);
    }
  }

  // Add knight moves
  b1 = parent.pieces[Knight][parent.turn] & ~pinned;
  while (b1) {
    s = BitBoardPopLSB(&b1);
    b2 = LookupTableGetKnightAttacks(l, s) & ~parent.occupancies[parent.turn];
    while (b2) children[numChildren++] = makeMove(parent, Knight, s, BitBoardPopLSB(&b2));
  }

  // Add bishop moves
  b1 = parent.pieces[Bishop][parent.turn] & ~pinned;
  while (b1) {
    s = BitBoardPopLSB(&b1);
    b2 = LookupTableGetBishopAttacks(l, s, parent.occupancies[Union]) & ~parent.occupancies[parent.turn];
    while (b2) children[numChildren++] = makeMove(parent, Bishop, s, BitBoardPopLSB(&b2));
  }

  // Add rook moves
  b1 = parent.pieces[Rook][parent.turn] & ~pinned;
  while (b1) {
    s = BitBoardPopLSB(&b1);
    b2 = LookupTableGetRookAttacks(l, s, parent.occupancies[Union]) & ~parent.occupancies[parent.turn];
    while (b2) children[numChildren++] = makeMove(parent, Rook, s, BitBoardPopLSB(&b2));
  }

  // Add queen moves
  b1 = parent.pieces[Queen][parent.turn] & ~pinned;
  while (b1) {
    s = BitBoardPopLSB(&b1);
    b2 = LookupTableGetQueenAttacks(l, s, parent.occupancies[Union]) & ~parent.occupancies[parent.turn];
    while (b2) children[numChildren++] = makeMove(parent, Queen, s, BitBoardPopLSB(&b2));
  }

  return;
}

// Note that the type of piece is meant to represent the piece that is on the
// square that was moved to, not the piece that was moved. It is meant to be used
// in the case of pawn promotion.
static inline ChessBoard makeMove(ChessBoard cb, Type t, Square from, Square to) {
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

  // Update castling bits
  cb.castling ^= cb.castling & BitBoardSetBit(EMPTY_BOARD, from);
  cb.castling ^= cb.castling & BitBoardSetBit(EMPTY_BOARD, to);

  // Handle enpassant move
  if (t == Pawn && offset == 1) {
    return makeMove(cb, Pawn, to, cb.enPassant);
  }

  // Update en passant square - double pawn push
  if (t == Pawn && offset > EDGE_SIZE + 1) {
    cb.enPassant = (from + to) / 2;
  } else {
    cb.enPassant = None;
  }

  // Handle castling move
  if (t == King && offset == 2) {
    Square ourRook = (to > from) ? from + 3 : from - 4;
    return makeMove(cb, Rook, ourRook, (to + from) / 2);
  }

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
  while (b1) attackedSquares |= LookupTableGetKingAttacks(l, BitBoardPopLSB(&b1));

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

// Given a parent and child chessboard, print the move that was played along with
// a given node count
void ChessBoardPrintMove(ChessBoard parent, ChessBoard child, long nodeCount) {
  Square from = BitBoardGetLSB((parent.occupancies[parent.turn] ^ child.occupancies[!child.turn]) & parent.occupancies[parent.turn]);
  Square to = BitBoardGetLSB((parent.occupancies[parent.turn] ^ child.occupancies[!child.turn]) & child.occupancies[!child.turn]);
  printf("%c%d%c%d: %ld\n", 'a' + BitBoardGetFile(from), EDGE_SIZE - BitBoardGetRank(from), 'a' +
                                  BitBoardGetFile(to), EDGE_SIZE - BitBoardGetRank(to), nodeCount);
}

bool ChessBoardIsEmpty(ChessBoard cb) {
  return !cb.occupancies[cb.turn];
}