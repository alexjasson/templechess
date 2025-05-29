#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"

// Bitboards representing the ranks from the perspective of the given color c
#define BACK_RANK(c) (BitBoard)((c == White) ? SOUTH_EDGE : NORTH_EDGE)
#define FEN_SIZE 128

static Color getColorFromASCII(char asciiColor);
static char getASCIIFromType(Type t, Color c);
static Type getTypeFromASCII(char asciiPiece);
static void addPiece(ChessBoard *cb, Square s, Type type);

// Assumes FEN is valid
ChessBoard ChessBoardNew(char *fen)
{
  ChessBoard cb;
  memset(&cb, 0, sizeof(ChessBoard));

  // Parse pieces and squares
  for (Square s = 0; s < BOARD_SIZE && *fen; fen++)
  {
    if (*fen == '/')
      continue;

    if (*fen >= '1' && *fen <= '8')
    {
      for (int numSquares = *fen - '0'; numSquares > 0; numSquares--)
      {
        cb.squares[s] = Empty;
        s++;
      }
    }
    else
    {
      Type t = getTypeFromASCII(*fen);
      Color c = isupper(*fen) ? White : Black;
      BitBoard b = BitBoardAdd(EMPTY_BOARD, s);
      cb.types[t] |= b;
      cb.colors[c] |= b;
      cb.squares[s] = t;
      s++;
    }
  }
  fen++;

  // Parse turn
  cb.turn = getColorFromASCII(*fen);
  fen += 2;

  // Parse castling
  if (*fen == '-')
  {
    fen += 2;
  }
  else
  {
    while (*fen != ' ')
    {
      Color c = (*fen == 'K' || *fen == 'Q') ? 0 : 7;
      BitBoard castlingSquares = (*fen == 'K' || *fen == 'k') ? KINGSIDE_CASTLING : QUEENSIDE_CASTLING;
      cb.castling |= (castlingSquares & BACK_RANK(c));
      fen++;
    }
    fen++;
  }

  // Parse en passant
  if (*fen != '-')
  {
    int file = *fen - 'a';
    fen++;
    int rank = EDGE_SIZE - (*fen - '0');
    cb.enPassant = rank * EDGE_SIZE + file;
  }

  return cb;
}

static Color getColorFromASCII(char asciiColor)
{
  return (asciiColor == 'w') ? White : Black;
}

// Returns the piece type represented by the FEN character (or Empty for '-')
static Type getTypeFromASCII(char asciiPiece)
{
  if (asciiPiece == '-')
    return Empty;
  static const char *pieces = "PKNBRQ";
  char *ptr = strchr(pieces, toupper(asciiPiece));
  if (ptr)
    return (Type)(ptr - pieces);
  return Empty;
}

// Returns the ASCII representation of a piece type given its color
static char getASCIIFromType(Type t, Color c)
{
  if (t == Empty)
    return '-';
  static const char *pieces = "PKNBRQ";
  char ch = pieces[t];
  return (c == Black) ? tolower(ch) : ch;
}

// Adds a piece of given type and current turn color to a chessboard; type=Empty clears the square
static void addPiece(ChessBoard *cb, Square s, Type type)
{
  BitBoard b = BitBoardAdd(EMPTY_BOARD, s);
  Type captured = cb->squares[s];
  if (captured != Empty) {
    cb->types[captured] &= ~b;
    cb->colors[White] &= ~b;
    cb->colors[Black] &= ~b;
  }
  if (type != Empty) {
    cb->types[type] |= b;
    cb->colors[cb->turn] |= b;
  }
  cb->squares[s] = type;
}

void ChessBoardPlayMove(ChessBoard *cb, Move m)
{
  // Update castling rights and clear en passant
  cb->castling &= ~(BitBoardAdd(EMPTY_BOARD, m.from.square) | BitBoardAdd(EMPTY_BOARD, m.to.square));
  cb->enPassant = EMPTY_SQUARE;

  // Pawn double push: set en passant target
  if (m.from.type == Pawn) {
    int diff = (int)m.to.square - (int)m.from.square;
    if (diff == 2 * EDGE_SIZE || diff == -2 * EDGE_SIZE)
      cb->enPassant = m.from.square + diff / 2;
  }

  // Remove captured piece
  if (m.captured.type != Empty)
    addPiece(cb, m.captured.square, Empty);

  // Move piece from origin to destination (handles promotion)
  addPiece(cb, m.from.square, Empty);
  addPiece(cb, m.to.square, m.to.type);

  // Castling: move rook if king moved two squares
  if (m.from.type == King) {
    int offset = (int)m.from.square - (int)m.to.square;
    if (offset == 2) {
      addPiece(cb, m.to.square - 2, Empty);
      addPiece(cb, m.to.square + 1, Rook);
    } else if (offset == -2) {
      addPiece(cb, m.to.square + 1, Empty);
      addPiece(cb, m.to.square - 1, Rook);
    }
  }

  // Toggle side to move
  cb->turn = !cb->turn;
}

void ChessBoardUndoMove(ChessBoard *cb, Move m)
{
  // Remove piece from destination
  addPiece(cb, m.to.square, Empty);

  // Restore captured piece
  if (m.captured.type != Empty)
    addPiece(cb, m.captured.square, m.captured.type);

  // Toggle side to move back
  cb->turn = !cb->turn;

  // Restore moved piece to origin
  addPiece(cb, m.from.square, m.from.type);

  // Undo castling: move rook back if king moved two squares
  if (m.from.type == King) {
    int offset = (int)m.from.square - (int)m.to.square;
    if (offset == 2) {
      addPiece(cb, m.to.square + 1, Empty);
      addPiece(cb, m.to.square - 2, Rook);
    } else if (offset == -2) {
      addPiece(cb, m.to.square - 1, Empty);
      addPiece(cb, m.to.square + 1, Rook);
    }
  }

  // Restore en passant and castling rights
  cb->enPassant = m.enPassant;
  cb->castling = m.castling;
}

void ChessBoardPrintBoard(ChessBoard cb)
{
  for (int rank = 0; rank < EDGE_SIZE; rank++)
  {
    for (int file = 0; file < EDGE_SIZE; file++)
    {
      Square s = rank * EDGE_SIZE + file;
      Type t = cb.squares[s];
      BitBoard b = BitBoardAdd(EMPTY_BOARD, s);
      Color c = (cb.colors[White] & b) ? White : Black;
      printf("%c ", getASCIIFromType(t, c));
    }
    printf("%d\n", EDGE_SIZE - rank);
  }
  printf("a b c d e f g h\n\n");
}

void ChessBoardPrintMove(Move m)
{
  printf("%c%d%c%d",
         'a' + (m.from.square % EDGE_SIZE),
         EDGE_SIZE - (m.from.square / EDGE_SIZE),
         'a' + (m.to.square % EDGE_SIZE),
         EDGE_SIZE - (m.to.square / EDGE_SIZE));
}

ChessBoard ChessBoardFlip(ChessBoard *cb)
{
  ChessBoard new;
  memcpy(&new, cb, sizeof(ChessBoard));
  new.turn = !new.turn;
  new.enPassant = EMPTY_SQUARE;
  return new;
}

char *ChessBoardToFEN(ChessBoard *cb)
{
  static char fen[FEN_SIZE];
  int index = 0;

  // 1) Piece placement from rank 8 down to rank 1 (which in your indexing is rank=0..7)
  for (int rank = 0; rank < EDGE_SIZE; rank++)
  {
    int emptyCount = 0;

    for (int file = 0; file < EDGE_SIZE; file++)
    {
      Square s = rank * EDGE_SIZE + file;
      Type t = cb->squares[s];

      if (t == Empty)
        emptyCount++; // If empty, just count up
      else
      {
        if (emptyCount > 0)
        {
          fen[index++] = '0' + emptyCount;
          emptyCount = 0;
        }
        BitBoard b = BitBoardAdd(EMPTY_BOARD, s);
        Color c = (cb->colors[White] & b) ? White : Black;
        fen[index++] = getASCIIFromType(t, c);
      }
    }

    // Flush remaining empty squares in this rank
    if (emptyCount > 0)
      fen[index++] = '0' + emptyCount;

    // Add the rank separator if it's not the last rank
    if (rank < EDGE_SIZE - 1)
      fen[index++] = '/';
  }

  // 2) Space + Active color: 'w' or 'b'
  fen[index++] = ' ';
  fen[index++] = (cb->turn == White) ? 'w' : 'b';
  fen[index++] = ' ';

  // We track whether we have found any castling rights
  int castlingCount = 0;

  // White kingside
  if ((cb->castling & (KINGSIDE_CASTLING & BACK_RANK(White))) == (KINGSIDE_CASTLING & BACK_RANK(White)))
  {
    fen[index++] = 'K';
    castlingCount++;
  }

  // White queenside
  if ((cb->castling & (QUEENSIDE_CASTLING & BACK_RANK(White))) == (QUEENSIDE_CASTLING & BACK_RANK(White)))
  {
    fen[index++] = 'Q';
    castlingCount++;
  }

  // Black kingside
  if ((cb->castling & (KINGSIDE_CASTLING & BACK_RANK(Black))) == (KINGSIDE_CASTLING & BACK_RANK(Black)))
  {
    fen[index++] = 'k';
    castlingCount++;
  }

  // Black queenside
  if ((cb->castling & (QUEENSIDE_CASTLING & BACK_RANK(Black))) == (QUEENSIDE_CASTLING & BACK_RANK(Black)))
  {
    fen[index++] = 'q';
    castlingCount++;
  }

  if (castlingCount == 0)
    fen[index++] = '-';
  fen[index++] = ' ';

  // 4) En passant target square (if any)
  if (cb->enPassant == EMPTY_SQUARE)
    fen[index++] = '-';
  else
  {
    int epFile = cb->enPassant % EDGE_SIZE; // 0..7
    int epRank = cb->enPassant / EDGE_SIZE; // 0..7, top row=0 => '8', bottom row=7 => '1'

    fen[index++] = 'a' + epFile;               // file letter
    fen[index++] = '0' + (EDGE_SIZE - epRank); // rank in FEN is (8 - epRank)
  }

  // 5) Null-terminate the string
  fen[index] = '\0';
  return fen;
}
