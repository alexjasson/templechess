#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ChessBoard.h"

#define PIECE_SIZE 12
#define GET_PIECE(t, c) ((t << 1) | c)
#define GET_TYPE(p) (p >> 1)
#define GET_COLOR(p) (p & 1)

#define MAX_CHILDREN 218

typedef uint8_t Piece;

static Color getColorFromASCII(char asciiColor);
static Piece getPieceFromASCII(char asciiPiece);
static char getASCIIFromPiece(Piece p);
static ChessBoard newChessBoard(void);
static ChessBoard makeMove(ChessBoard cb, Type t, Square from, Square to); // ie set data
// static void setMetadata(ChessBoard *cb, LookupTable l);

static ChessBoard newChessBoard(void) {
  ChessBoard cb;
  memset(&cb, 0, sizeof(ChessBoard));
  return cb;
}

// Assumes FEN is valid
ChessBoard ChessBoardFromFEN(char *fen, LookupTable l) {
  ChessBoard cb = newChessBoard();

  // Parse pieces
  Square s = a8;
  while (s <= h1 && *fen) {
    if (*fen == '/') {
      fen++;
    } else if (*fen >= '1' && *fen <= '8') {
      s += *fen - '0';
      fen++;
    } else {
      Piece p = getPieceFromASCII(*fen);
      Type t = GET_TYPE(p); Color c = GET_COLOR(p);
      cb.pieces[t][c] = BitBoardSetBit(cb.pieces[t][c], s);
      cb.occupancies[c] = BitBoardSetBit(cb.occupancies[c], s);
      cb.occupancies[Union] = BitBoardSetBit(cb.occupancies[Union], s);
      s++;
      fen++;
    }
  }
  fen++;

  cb.turn = getColorFromASCII(*fen);

  fen++;
  fen++;
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
  }
  fen++;

  if (*fen != '-') {
    int file = *fen - 'a';
    fen++;
    int rank = *fen - '0';
    cb.enPassant = rank * EDGE_SIZE + file;
  }

    for (Color c = White; c <= Black; c++) {
    for (Type t = Pawn; t <= Queen; t++) {
      cb.occupancies[c] |= cb.pieces[t][c];
    }
    cb.occupancies[Union] |= cb.occupancies[c];
  }

  // Attacks
  for (Color c = White; c <= Black; c++) {
    for (Type t = Pawn; t <= Queen; t++) {
      BitBoard pieces = cb.pieces[t][c];
      while (pieces) {
        Square s = BitBoardLeastSignificantBit(pieces);
        cb.attacks[c] |= LookupTableGetPieceAttacks(l, s, t, c, cb.occupancies[Union]);
        pieces = BitBoardPopBit(pieces, s);
      }
    }
  }
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
  ChessBoard *children = malloc(MAX_CHILDREN * sizeof(ChessBoard));
  int numChildren = 0;
  for (Type t = Pawn; t <= Queen; t++) {
    BitBoard pieces = cb.pieces[t][cb.turn];
    while (pieces) {
      Square s = BitBoardLeastSignificantBit(pieces);
      // Get attacks
      BitBoard attacks = LookupTableGetPieceAttacks(l, s, t, cb.turn, cb.occupancies[Union]);
      attacks &= ~cb.occupancies[cb.turn];
      if (t == Pawn) attacks &= cb.occupancies[!cb.turn];
      // Get moves
      // Add function here
      // I think should just be one lookup with some moves pruned
      while (attacks) {
        Square a = BitBoardLeastSignificantBit(attacks);
        ChessBoard newBoard = makeMove(cb, t, s, a);

        children[numChildren++] = newBoard;
        attacks = BitBoardPopBit(attacks, a);
      }
      pieces = BitBoardPopBit(pieces, s);
    }
  }
  return children;
}

static ChessBoard makeMove(ChessBoard cb, Type t, Square from, Square to) {
  int offset = abs((int)(from - to));

  // Remove piece from from square and set it to to square
  cb.pieces[t][cb.turn] = BitBoardSetBit(BitBoardPopBit(cb.pieces[t][cb.turn], from), to);
  // Remove piece from to square if it exists
  for (Type t = Pawn; t <= Queen; t++) {
    cb.pieces[t][!cb.turn] = BitBoardPopBit(cb.pieces[t][!cb.turn], to);
  }

  // Occupancies can be calculated quickly here without looping over pieces
  cb.occupancies[cb.turn] = BitBoardSetBit(BitBoardPopBit(cb.occupancies[cb.turn], from), to);
  cb.occupancies[!cb.turn] = BitBoardPopBit(cb.occupancies[!cb.turn], to);
  cb.occupancies[Union] = BitBoardSetBit(BitBoardPopBit(cb.occupancies[Union], from), to);


  // Update castling
  cb.castling ^= cb.castling & BitBoardSetBit(EMPTY_BOARD, from);
  // Update en passant square
  cb.enPassant = (t == Pawn && offset > EDGE_SIZE + 1) ? (from + to) / 2 : UNDEFINED;



  // if (t == Pawn && offset == 1) {
  //   // Handle enPassant - make move again
  // } else if (t == King && offset == 2) {
  //   // Handle castling - make move again
  // }

  // Update color
  cb.turn = !cb.turn;
  return cb;
}

// setMetadata
// static void setMetadata(ChessBoard *cb, LookupTable l) {
//   // Occupancies
// }