#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ChessBoard.h"

#define PIECE_SIZE 12
#define GET_PIECE(t, c) ((t << 1) | c)
#define GET_TYPE(p) (p >> 1)
#define GET_COLOR(p) (p & 1)

typedef uint8_t Piece;

static Color getColorFromASCII(char asciiColor);
static Piece getPieceFromASCII(char asciiPiece);
static char getASCIIFromPiece(Piece p);

ChessBoard ChessBoardFromFEN(char *fen, LookupTable l) {
  ChessBoard board;
  memset(board.pieces, EMPTY_BOARD, PIECE_SIZE * sizeof(BitBoard));
  memset(board.occupancies, EMPTY_BOARD, (COLOR_SIZE + 1) * sizeof(BitBoard));
  memset(board.pieceAttacks, EMPTY_BOARD, PIECE_SIZE * sizeof(BitBoard));

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
      board.pieces[t][c] = BitBoardSetBit(board.pieces[t][c], s);
      board.occupancies[c] = BitBoardSetBit(board.occupancies[c], s);
      board.occupancies[Union] = BitBoardSetBit(board.occupancies[Union], s);
      s++;
      fen++;
    }
  }

  if (*fen != ' ') {
    printf("Invalid FEN!\n");
    exit(1);
  }
  fen++;

  board.turn = getColorFromASCII(*fen);
  for (Type t = Pawn; t <= Queen; t++) {
    for (Color c = White; c <= Black; c++) {
      BitBoard pieces = board.pieces[t][c];
      while (pieces) {
        Square s = BitBoardLeastSignificantBit(pieces);
        board.pieceAttacks[t][c] |= LookupTableGetPieceAttacks(l, s, t, c, board.occupancies[Union]);
        pieces = BitBoardPopBit(pieces, s);
      }
    }
  }

  return board;
}

void ChessBoardPrint(ChessBoard board) {
  for (int rank = 0; rank < EDGE_SIZE; rank++) {
    for (int file = 0; file < EDGE_SIZE; file++) {
      Square s = (EDGE_SIZE + rank) * EDGE_SIZE + file;
      for (Piece p = 0; p < PIECE_SIZE; p++) {
        if (BitBoardGetBit(board.pieces[GET_TYPE(p)][GET_COLOR(p)], s)) {
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

// Assumes that FEN is valid
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
ChessBoard *ChessBoardGetChildren(ChessBoard board, LookupTable l) {
  ChessBoard *children = malloc(218 * sizeof(ChessBoard));
  int numChildren = 0;
  for (Type t = Pawn; t <= Queen; t++) {
    BitBoard pieces = board.pieces[t][board.turn];
    while (pieces) {
      Square s = BitBoardLeastSignificantBit(pieces);
      // Get attacks
      BitBoard attacks = LookupTableGetPieceAttacks(l, s, t, board.turn, board.occupancies[Union]);
      attacks &= ~board.occupancies[board.turn];
      if (t == Pawn) attacks &= board.occupancies[!board.turn];
      // Get moves
      // Add function here
      while (attacks) {
        Square a = BitBoardLeastSignificantBit(attacks);
        ChessBoard newBoard = board;  // Copy the original board

        newBoard.pieces[t][board.turn] = BitBoardSetBit(BitBoardPopBit(newBoard.pieces[t][board.turn], s), a);
        newBoard.occupancies[board.turn] = BitBoardSetBit(BitBoardPopBit(newBoard.occupancies[board.turn], s), a);
        newBoard.occupancies[Union] = BitBoardSetBit(BitBoardPopBit(newBoard.occupancies[Union], s), a);
        newBoard.turn = !board.turn;
        newBoard.pieceAttacks[t][board.turn] |= LookupTableGetPieceAttacks(l, a, t, board.turn, newBoard.occupancies[Union]);

        children[numChildren++] = newBoard;
        attacks = BitBoardPopBit(attacks, a);
      }
      pieces = BitBoardPopBit(pieces, s);
    }
  }
  return children;
}