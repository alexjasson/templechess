#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"

#define FEN_SIZE 128

static Color getColorFromASCII(char asciiColor);
static char getASCIIFromType(Type t, Color c);
static Type getTypeFromASCII(char asciiPiece);

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


void ChessBoardPlayMove(ChessBoard *cb, Move m)
{
  BitBoard fromBit = BitBoardAdd(EMPTY_BOARD, m.from.square);
  BitBoard toBit   = BitBoardAdd(EMPTY_BOARD, m.to.square);
  Color us = cb->turn;

  // Update castling rights and clear en passant
  cb->castling &= ~(fromBit | toBit);
  cb->enPassant = EMPTY_SQUARE;

  // Pawn double push: set en passant square
  if (m.from.type == Pawn) {
    int diff = (int)m.to.square - (int)m.from.square;
    if (diff == 2 * EDGE_SIZE || diff == -2 * EDGE_SIZE)
      cb->enPassant = m.from.square + diff / 2;
  }

  // Remove captured piece
  if (m.captured.type != Empty) {
    BitBoard capBit = BitBoardAdd(EMPTY_BOARD, m.captured.square);
    cb->types[m.captured.type] ^= capBit;
    cb->colors[!us] ^= capBit;
    cb->squares[m.captured.square] = Empty;
  }

  // Move piece: remove from origin, place at destination
  cb->types[m.from.type] ^= fromBit;
  cb->types[m.to.type]   ^= toBit;
  cb->colors[us] ^= fromBit | toBit;
  cb->squares[m.from.square] = Empty;
  cb->squares[m.to.square]   = m.to.type;

  // Castling: move rook if king moved two squares
  if (m.from.type == King) {
    int offset = (int)m.from.square - (int)m.to.square;
    if (offset == 2 || offset == -2) {
      Square rookFrom = (offset == 2) ? m.to.square - 2 : m.to.square + 1;
      Square rookTo   = (offset == 2) ? m.to.square + 1 : m.to.square - 1;
      BitBoard rookBits = BitBoardAdd(EMPTY_BOARD, rookFrom) | BitBoardAdd(EMPTY_BOARD, rookTo);
      cb->types[Rook]  ^= rookBits;
      cb->colors[us]   ^= rookBits;
      cb->squares[rookFrom] = Empty;
      cb->squares[rookTo]   = Rook;
    }
  }

  // Toggle side to move
  cb->turn = !cb->turn;
}

void ChessBoardUndoMove(ChessBoard *cb, Move m)
{
  BitBoard fromBit = BitBoardAdd(EMPTY_BOARD, m.from.square);
  BitBoard toBit   = BitBoardAdd(EMPTY_BOARD, m.to.square);

  // Toggle side to move back
  cb->turn = !cb->turn;
  Color us = cb->turn;

  // Undo castling: move rook back if king moved two squares
  if (m.from.type == King) {
    int offset = (int)m.from.square - (int)m.to.square;
    if (offset == 2 || offset == -2) {
      Square rookFrom = (offset == 2) ? m.to.square - 2 : m.to.square + 1;
      Square rookTo   = (offset == 2) ? m.to.square + 1 : m.to.square - 1;
      BitBoard rookBits = BitBoardAdd(EMPTY_BOARD, rookFrom) | BitBoardAdd(EMPTY_BOARD, rookTo);
      cb->types[Rook]  ^= rookBits;
      cb->colors[us]   ^= rookBits;
      cb->squares[rookTo]   = Empty;
      cb->squares[rookFrom] = Rook;
    }
  }

  // Move piece back: remove from destination, place at origin
  cb->types[m.to.type]   ^= toBit;
  cb->types[m.from.type] ^= fromBit;
  cb->colors[us] ^= fromBit | toBit;
  cb->squares[m.to.square]   = Empty;
  cb->squares[m.from.square] = m.from.type;

  // Restore captured piece
  if (m.captured.type != Empty) {
    BitBoard capBit = BitBoardAdd(EMPTY_BOARD, m.captured.square);
    cb->types[m.captured.type] ^= capBit;
    cb->colors[!us] ^= capBit;
    cb->squares[m.captured.square] = m.captured.type;
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

int ChessBoardCount(LookupTable l, ChessBoard *cb)
{
  int count = 0;
  Square s;
  BitBoard attacked = ChessBoardAttacked(l, cb);
  BitBoard pinned, checking;
  ChessBoardCheckingAndPinned(l, cb, &checking, &pinned);

  // Cache hot values
  const BitBoard us    = ChessBoardUs(cb);
  const BitBoard them  = ChessBoardThem(cb);
  const BitBoard all   = ChessBoardAll(cb);
  const BitBoard kingB = ChessBoardOur(cb, King);
  const Square  kingSq = BitBoardPeek(kingB);
  const int     color  = ChessBoardColor(cb);

  // Determine check mask
  BitBoard checkMask;
  const int numChecks = BitBoardCount(checking);
  if (numChecks == 0) {
    checkMask = ~EMPTY_BOARD;
  } else if (numChecks == 1) {
    Square cs = BitBoardPeek(checking);
    checkMask = BitBoardAdd(EMPTY_BOARD, cs) | LookupTableSquaresBetween(l, kingSq, cs);
  } else {
    checkMask = EMPTY_BOARD;
  }

  // King moves
  BitBoard moves = LookupTableAttacks(l, kingSq, King, EMPTY_BOARD) & ~us & ~attacked;
  if (numChecks == 0) {
    // Kingside: check rights and interior squares f/g
    int clear = (((attacked & ATTACK_MASK) | (all & OCCUPANCY_MASK)) &
                 (KINGSIDE & ~KINGSIDE_CASTLING) & BACK_RANK(color)) == EMPTY_BOARD;
    if (ChessBoardKingSide(cb) && clear) moves |= (kingB << 2);

    // Queenside: check rights and interior squares b/c/d
    clear = (((attacked & ATTACK_MASK) | (all & OCCUPANCY_MASK)) &
             (QUEENSIDE & ~QUEENSIDE_CASTLING) & BACK_RANK(color)) == EMPTY_BOARD;
    if (ChessBoardQueenSide(cb) && clear) moves |= (kingB >> 2);
  }
  count += BitBoardCount(moves);

  // If double-check, return early (only king moves allowed)
  if (numChecks == 2) return count;

  const BitBoard notUsAndCheck = ~us & checkMask;

  // Knight moves
  BitBoard piecesKnight = ChessBoardOur(cb, Knight);
  while (piecesKnight) {
    s = BitBoardPop(&piecesKnight);
    moves = LookupTableAttacks(l, s, Knight, all) & notUsAndCheck;
    if (BitBoardAdd(EMPTY_BOARD, s) & pinned)
      moves &= LookupTableLineOfSight(l, kingSq, s);
    count += BitBoardCount(moves);
  }

  // Diagonal slider moves (bishops + queens)
  BitBoard diagSliders = ChessBoardOur(cb, Bishop) | ChessBoardOur(cb, Queen);
  while (diagSliders) {
    s = BitBoardPop(&diagSliders);
    moves = LookupTableAttacks(l, s, Bishop, all) & notUsAndCheck;
    if (BitBoardAdd(EMPTY_BOARD, s) & pinned)
      moves &= LookupTableLineOfSight(l, kingSq, s);
    count += BitBoardCount(moves);
  }

  // Orthogonal slider moves (rooks + queens)
  BitBoard orthoSliders = ChessBoardOur(cb, Rook) | ChessBoardOur(cb, Queen);
  while (orthoSliders) {
    s = BitBoardPop(&orthoSliders);
    moves = LookupTableAttacks(l, s, Rook, all) & notUsAndCheck;
    if (BitBoardAdd(EMPTY_BOARD, s) & pinned)
      moves &= LookupTableLineOfSight(l, kingSq, s);
    count += BitBoardCount(moves);
  }

  // Pawn moves
  const BitBoard promotion = BACK_RANK(White) | BACK_RANK(Black);
  const BitBoard enpassant = ENPASSANT_RANK(color);
  const BitBoard ourPawns  = ChessBoardOur(cb, Pawn);

  BitBoard b1 = ourPawns & ~pinned;
  moves = PAWN_ATTACKS_LEFT(b1, color) & them & checkMask;
  count += BitBoardCount(moves) + BitBoardCount(moves & promotion) * 3;
  moves = PAWN_ATTACKS_RIGHT(b1, color) & them & checkMask;
  count += BitBoardCount(moves) + BitBoardCount(moves & promotion) * 3;
  BitBoard b2 = SINGLE_PUSH(b1, color) & ~all;
  moves = b2 & checkMask;
  count += BitBoardCount(moves) + BitBoardCount(moves & promotion) * 3;
  moves = SINGLE_PUSH(b2 & enpassant, color) & ~all & checkMask;
  count += BitBoardCount(moves);

  b1 = ourPawns & pinned;
  while (b1) {
    s = BitBoardPop(&b1);
    BitBoard b3 = LookupTableLineOfSight(l, kingSq, s);
    moves  = PAWN_ATTACKS(BitBoardAdd(EMPTY_BOARD, s), color) & them;
    b2     = SINGLE_PUSH(BitBoardAdd(EMPTY_BOARD, s), color) & ~all;
    moves |= b2;
    moves |= SINGLE_PUSH(b2 & enpassant, color) & ~all;
    count += BitBoardCount(moves & b3 & checkMask)
           + BitBoardCount((moves & b3 & checkMask) & promotion) * 3;
  }

  // En passant moves
  if (ChessBoardEnPassant(cb) != EMPTY_SQUARE) {
    BitBoard epSq = BitBoardAdd(EMPTY_BOARD, ChessBoardEnPassant(cb));
    b1 = PAWN_ATTACKS(epSq, !color) & ourPawns;
    b2 = EMPTY_BOARD;
    while (b1) {
      s = BitBoardPop(&b1);

      // Pseudo-pin check for en passant
      if (LookupTableAttacks(l, kingSq, Rook,
            all & ~BitBoardAdd(SINGLE_PUSH(epSq, !color), s)) &
          RANK_OF(kingSq) & (ChessBoardTheir(cb, Rook) | ChessBoardTheir(cb, Queen)))
        continue;

      b2 |= BitBoardAdd(EMPTY_BOARD, s);
      if (b2 & pinned)
        b2 &= LookupTableLineOfSight(l, kingSq, ChessBoardEnPassant(cb));
    }
    if (b2 != EMPTY_BOARD)
      count += BitBoardCount(b2);
  }

  return count;
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

void ChessBoardCheckingAndPinned(LookupTable l, ChessBoard *cb, BitBoard *checking, BitBoard *pinned)
{
  Square ourKing = BitBoardPeek(ChessBoardOur(cb, King));

  *checking = (PAWN_ATTACKS(ChessBoardOur(cb, King), ChessBoardColor(cb)) & ChessBoardTheir(cb, Pawn)) |
              (LookupTableAttacks(l, ourKing, Knight, EMPTY_BOARD) & ChessBoardTheir(cb, Knight));
  *pinned = EMPTY_BOARD;

  BitBoard candidates = (LookupTableAttacks(l, ourKing, Bishop, ChessBoardThem(cb)) & (ChessBoardTheir(cb, Bishop) |
                         ChessBoardTheir(cb, Queen))) | (LookupTableAttacks(l, ourKing, Rook, ChessBoardThem(cb)) &
                        (ChessBoardTheir(cb, Rook) | ChessBoardTheir(cb, Queen)));

  while (candidates)
  {
    Square s = BitBoardPop(&candidates);
    BitBoard b = LookupTableSquaresBetween(l, ourKing, s) & ChessBoardAll(cb) & ~ChessBoardThem(cb);
    if (b == EMPTY_BOARD)
      *checking |= BitBoardAdd(EMPTY_BOARD, s);
    else if ((b & (b - 1)) == EMPTY_BOARD)
      *pinned |= b;
  }
}

BitBoard ChessBoardAttacked(LookupTable l, ChessBoard *cb)
{
  BitBoard occupancies = ChessBoardAll(cb) & ~ChessBoardOur(cb, King);
  BitBoard attacked = PAWN_ATTACKS(ChessBoardTheir(cb, Pawn), !ChessBoardColor(cb));
  BitBoard b = ChessBoardTheir(cb, Knight);
  while (b) attacked |= LookupTableAttacks(l, BitBoardPop(&b), Knight, occupancies);
  b = ChessBoardTheir(cb, Bishop);
  while (b) attacked |= LookupTableAttacks(l, BitBoardPop(&b), Bishop, occupancies);
  b = ChessBoardTheir(cb, Rook);
  while (b) attacked |= LookupTableAttacks(l, BitBoardPop(&b), Rook, occupancies);
  b = ChessBoardTheir(cb, Queen);
  while (b) attacked |= LookupTableAttacks(l, BitBoardPop(&b), Queen, occupancies);
  b = ChessBoardTheir(cb, King);
  while (b) attacked |= LookupTableAttacks(l, BitBoardPop(&b), King, occupancies);
  return attacked;
}
