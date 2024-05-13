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

- UPDATE: It is very likely that calculating pawn moves OTF is simply faster in most positions
  than having a look up table for pawns. Especially considering this lookup table will eat up
  L1 cache. Hence, pawn moves should be implemented OTF.

To traverse left pawn moves:
- play all left pawn attacks on pawn bitboard
- Pop the moves in a while loop to get the to square
- Use BitBoardSetBit on the square to get a bitboard with the to square
- Shift the to bitboard left from the opponents perspective to get the from bitboard
- Use BitBoardGetLSB on the from bitboard to get the from square
- You now have a bitboard with a to bit and a from square
- Pass this to the traverseFn
- Continue loop

- If we're counting moves in the tree search function, we don't need to traverse each
  pawn move like above, we can ust get a bitboard of all the pawn moves (left, right, single push, double push)
  and then count the moves on it. Since we're not actually playing these moves on our
  board rep.

- Consider using a goto for numChecking in treeSearch to reduce nested if statements

- Get the checking piece type?

- Put regular pawn moves, piece moves and promotion moves in their own function? (regular meaning non pinned)
  Since these are repeated twice.