# templechess

TODO:

- A lookup table for all pawn moves including en passant, the key for
  the lookup table will be the occupancies with the enpassant square
  encoded on the back rank. Remember that for any given square a pawn
  only has a maximum of 4 possible moves. En passant will be encoded
  as a move to the side. For the pawn, all bits are relevant. For example,
  if we're a pawn on the b file, we want to know if we can move (attack) on file a.
- Have a pawnMove function that handles all pawn moves, can pass it a
  piece type to handle promotion. Enpassant will be handled recursively.
  Double pushes will be detected by square differences. During move traversal
  promotions will be handled by splitting 'to' bits into separate bitboards for
  promotion rank and ~promotion rank.
- The relevant bits of a white pawn on the 4th rank would be the 3 bits in front of it
  and the 8 bits on the back rank (for enPassant). However the 8 bits on the backrank need
  only be a power set where each set has cardinality 1 or empty. So we have 3^2 * 9 = 72
  maximum possible 'occupancy' sets for any given pawn. We will need some function to generate
  subsets of maximum cardinality 1, since currently we only generate all subsets. That means the
  pawns lookup table will take up 72 * 64 * 2 * 8 = ~74KB.
  ^DEPRECIATED^

- Change BitBoard enPassant to BitRank enPassant in ChessBoard ADT
- Change BitBoard ADT file to BitMap ADT, use BitMap in place of BitBoard
- A general move lookup function given any piece. Can use function pointers to
  index to the relevant function using a piece as the index. This should be the only
  function that is shown to the user of ADT for piece attacks/moves.
- Make makeMove function faster by passing the piece that is moving to the function.
  the piece will be given by the squares array while we are determining the attacks.
  Aside from makeMove and pawnMove, the only other move functions should be kingsideCastling
  and queensideCastling. kingsideCastling can be easily done with simple bit shifts of the
  BitRanks and addition/subtraction of the squares.
- Change the function getAttackedSquares so that you just loop over all the pieces that
  are not pawns and then use the general attack lookup function rather than individually
  adding king moves, then knight moves then orthogonal sliding moves and diagonal sliding
  moves. Remember to use this general approach for legal move generation as well because
  it has no down side.
- A function for setting two bits on a bitboard might be useful.
- Use this algorithm when traversing castling:
  If (castling.rank >= QUEENSIDE_CASTLING || castling.rank == KINGSIDE_CASTLING) {
    // We have castling rights
    // We now OR castling.rank with the attack/occupancy bits
    If (castling.rank == BOTHSIDE_CASTLING) {
      makeQueensideCastling...
      makeKingsideCastling...
    } else if (castling.rank & KINGSIDE == KINGSIDE_CASTLING) {
      makeKingsideCastling...
    } else {
      makeQueensideCastling...
    }
  }
- Note that PEXT bitboards are slower if the CPU does not support the PEXT instruction. Should
  print a warning when compiling if the users CPU doesn't support the PEXT instruction.
- Consider passing a more generic function to treeSearch. Could make the argument of the
  traverseFn a void pointer instead.

- UPDATE: It is very likely that calculating pawn moves OTF is simply faster in most positions
  than having a look up table for pawns. Especially considering this lookup table will eat up
  L1 cache. Hence, pawn moves should be implemented OTF.

- To simplify we may be able remove the queen bitboard from pieces array in chessboard struct.
  Then the queen would just be added to both the rook/bishop bitboards