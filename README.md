# templechess
Templechess is a single-threaded chess move generator written in C with no hash table and no SIMD instructions. With my Intel i5-10210U Processor (max 4.2GHz), it completes perft(7) from the starting position in ~11.3 seconds, which equates to ~280M nps. It is currently under 900 LOC, including tests and Makefile.

It assumes a UNIX-like operating system such as linux or mac and would need to be modified to compile on windows.

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
./test
```

![](perft(7).png)
