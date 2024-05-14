# templechess
A single threaded chess move generator written in C with no hash table. With my Intel i5-10210U Processor (max 4.2GHz) it completes perft(7) from the starting position in ~8.5 seconds which equates to ~376M nps. Comparatively Stockfish 16.1 completes it in ~21 seconds.

## Compilation
```
cd src
make
```

## Usage
To run the perft:
```
./perft <FEN> <depth>
```
To run the tests:
```
./testChessBoard
```

![](perft(7).png)
