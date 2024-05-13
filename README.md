# templechess

TODO:

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

- If we're counting moves in the tree search function, we don't need to traverse each
  pawn move like above, we can ust get a bitboard of all the pawn moves (left, right, single push, double push)
  and then count the moves on it. Since we're not actually playing these moves on our
  board rep.