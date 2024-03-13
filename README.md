# templechess

TODO:

- A lookup table for all pawn moves including en passant, the key for
  the lookup table will be the occupancies with the enpassant square
  encoded on the back rank

- Change BitBoard ADT to BitMap ADT
- Have a pawnMove function that handles all pawn moves, can pass it a
  piece type to handle promotion. Enpassant will be handled recursively.
- A general move lookup function given any piece. Can use function pointers to
  index to the relevant function using a piece as the index.
- Make makeMove function faster by passing the piece that is moving to the function.
  the piece will be given by the squares array while we are determining the attacks