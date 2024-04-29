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

- UPDATE: It is very likely that calculating pawn moves OTF is simply faster in most positions
  than having a look up table for pawns. Especially considering this lookup table will eat up
  L1 cache. Hence, pawn moves should be implemented OTF.

- To traverse pawn attacks, play all left pawn attacks on pawn bitboard, then the moves are just
  the left pawn attacks of the opponent with the squares reversed