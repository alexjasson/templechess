#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"

// Returns a bitboard representing a set of moves given a set of pawns and a color
#define PAWN_ATTACKS(b, c) ((c == White) ? BitBoardShiftNW(b) | BitBoardShiftNE(b) : BitBoardShiftSW(b) | BitBoardShiftSE(b))

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
      cb.pieceTypeBB[t] |= b;
      cb.colorBB[c] |= b;
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
// Returns 1 if the side to move has king-side castling rights, 0 otherwise
int ChessBoardKingSide(ChessBoard *cb)
{
  BitBoard mask = KINGSIDE_CASTLING & BACK_RANK(cb->turn);
  return ((cb->castling & mask) == mask) ? 1 : 0;
}
// Returns 1 if the side to move has queen-side castling rights, 0 otherwise
int ChessBoardQueenSide(ChessBoard *cb)
{
  BitBoard mask = QUEENSIDE_CASTLING & BACK_RANK(cb->turn);
  return ((cb->castling & mask) == mask) ? 1 : 0;
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

ChessBoard ChessBoardPlayMove(ChessBoard *old, Move m)
{
  ChessBoard new;
  memcpy(&new, old, sizeof(ChessBoard));
  int offset = m.from - m.to;
  new.enPassant = EMPTY_SQUARE;
  new.castling &= ~(BitBoardAdd(EMPTY_BOARD, m.from) | BitBoardAdd(EMPTY_BOARD, m.to));
  addPiece(&new, m.to, m.type);
  addPiece(&new, m.from, Empty);

  if (m.type == Pawn)
  {
    if ((offset == 16) || (offset == -16))
    { // Double push
      new.enPassant = m.from - (offset / 2);
    }
    else if (m.to == old->enPassant)
    { // En passant capture
      addPiece(&new, m.to + (new.turn ? -8 : 8), Empty);
    }
  }
  else if (m.type == King)
  {
    if (offset == 2)
    { // Queenside castling
      addPiece(&new, m.to - 2, Empty);
      addPiece(&new, m.to + 1, Rook);
    }
    else if (offset == -2)
    { // Kingside castling
      addPiece(&new, m.to + 1, Empty);
      addPiece(&new, m.to - 1, Rook);
    }
  }

  new.turn = !new.turn;

  return new;
}

// Adds a piece of given type and current turn color to a chessboard; type=Empty clears the square
static void addPiece(ChessBoard *cb, Square s, Type type)
{
  BitBoard b = BitBoardAdd(EMPTY_BOARD, s);
  Type captured = cb->squares[s];
  if (captured != Empty) {
    cb->pieceTypeBB[captured] &= ~b;
    cb->colorBB[White] &= ~b;
    cb->colorBB[Black] &= ~b;
  }
  if (type != Empty) {
    cb->pieceTypeBB[type] |= b;
    cb->colorBB[cb->turn] |= b;
  }
  cb->squares[s] = type;
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
      Color c = (cb.colorBB[White] & b) ? White : Black;
      printf("%c ", getASCIIFromType(t, c));
    }
    printf("%d\n", EDGE_SIZE - rank);
  }
  printf("a b c d e f g h\n\n");
}

void ChessBoardPrintMove(Move m)
{
  printf("%c%d%c%d", 'a' + (m.from % EDGE_SIZE), EDGE_SIZE - (m.from / EDGE_SIZE), 'a' + (m.to % EDGE_SIZE), EDGE_SIZE - (m.to / EDGE_SIZE));
}

BitBoard ChessBoardChecking(LookupTable l, ChessBoard *cb)
{
  Square ourKing = BitBoardPeek(OUR(King));
  BitBoard checking = (PAWN_ATTACKS(OUR(King), cb->turn) & THEIR(Pawn)) |
                      (LookupTableAttacks(l, ourKing, Knight, EMPTY_BOARD) & THEIR(Knight));
  BitBoard candidates = (LookupTableAttacks(l, ourKing, Bishop, THEM) & (THEIR(Bishop) | THEIR(Queen))) |
                        (LookupTableAttacks(l, ourKing, Rook, THEM) & (THEIR(Rook) | THEIR(Queen)));

  while (candidates)
  {
    Square s = BitBoardPop(&candidates);
    BitBoard b = LookupTableSquaresBetween(l, ourKing, s) & ALL & ~THEM;
    if (b == EMPTY_BOARD)
    {
      checking |= BitBoardAdd(EMPTY_BOARD, s);
    }
  }

  return checking;
}

BitBoard ChessBoardPinned(LookupTable l, ChessBoard *cb)
{
  Square ourKing = BitBoardPeek(OUR(King));
  BitBoard candidates = (LookupTableAttacks(l, ourKing, Bishop, THEM) & (THEIR(Bishop) | THEIR(Queen))) |
                        (LookupTableAttacks(l, ourKing, Rook, THEM) & (THEIR(Rook) | THEIR(Queen)));
  BitBoard pinned = EMPTY_BOARD;

  while (candidates)
  {
    Square s = BitBoardPop(&candidates);
    BitBoard b = LookupTableSquaresBetween(l, ourKing, s) & ALL & ~THEM;
    if (b != EMPTY_BOARD && (b & (b - 1)) == EMPTY_BOARD)
    {
      pinned |= b;
    }
  }

  return pinned;
}

BitBoard ChessBoardAttacked(LookupTable l, ChessBoard *cb)
{
  BitBoard attacked, b;
  BitBoard occupancies = ALL & ~OUR(King);

  attacked = PAWN_ATTACKS(THEIR(Pawn), (!cb->turn));
  b = THEM & ~THEIR(Pawn);
  while (b)
  {
    Square s = BitBoardPop(&b);
    attacked |= LookupTableAttacks(l, s, cb->squares[s], occupancies);
  }
  return attacked;
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
        Color c = (cb->colorBB[White] & b) ? White : Black;
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
