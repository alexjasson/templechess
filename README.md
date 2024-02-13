# templechess

TODO:

- Fix bug with "./perft 2 '8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -'" (g2g4)
- Create Move object where captures/quiet moves are even/odd and the MoveType
  is given by a bitwise right shift

example move encoding

0  
1 double push  
2 enpassant  
3  
4  
5 castling  
6 knight promotion capture  
7 knight promotion quiet  
8 bishop promotion capture  
9 bishop promotion quiet  
10 rook promotion capture  
11 rook promotion quiet  
12 queen promotion capture  
13 queen promotion quiet  
14 normal capture  
15 normal quiet  