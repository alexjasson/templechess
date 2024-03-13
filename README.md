# templechess

TODO:

- A lookup table for all pawn moves including en passant, the key for
  the lookup table will be the occupancies with the enpassant square
  encoded on the back rank. Remember that for any given square a pawn
  only has a maximum of 4 possible moves. En passant will be encoded
  as a move to the side. For the pawn, all bits are relevant. For example,
  if we're a pawn on the b file, we want to know if we can move (attack) on file a.
- Change BitBoard enPassant to BitRank enPassant in ChessBoard ADT
- Change BitBoard ADT file to BitMap ADT, use BitMap in place of BitBoard
- Have a pawnMove function that handles all pawn moves, can pass it a
  piece type to handle promotion. Enpassant will be handled recursively.
  Double pushes will be detected by square differences. During move traversal
  promotions will be handled by splitting 'to' bits into separate bitboards for
  promotion rank and ~promotion rank.
- A general move lookup function given any piece. Can use function pointers to
  index to the relevant function using a piece as the index. This should be the only
  function that is shown to the user of ADT for piece attacks/moves.
- Make makeMove function faster by passing the piece that is moving to the function.
  the piece will be given by the squares array while we are determining the attacks.
  Aside from makeMove and pawnMove, the only other move functions should be kingsideCastling
  and queensideCastling.
- Change the function getAttackedSquares so that you just loop over all the pieces that
  are not pawns and then use the general attack lookup function rather than individually
  adding king moves, then knight moves then orthogonal sliding moves and diagonal sliding
  moves. Remember to use this general approach for legal move generation as well because
  it has no down side.
- A function for setting two bits on a bitboard might be useful.