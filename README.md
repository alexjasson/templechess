# templechess

This is an open source chess move generator written in C. The goal of the project is to count the number of chess positions (nodes) reachable from a given position as fast as possible on a CPU. It is currently single-threaded and does not use a transposition table. On an Intel i5-8400 (up to 4.0 GHz), it completes perft(7) in ~3.2 s, corresponding to ~1 billion nodes per second.

## Compilation

Prerequisites:

- A UNIX-like OS (Linux or mac)
- GCC
- make

From the project root, run:

```bash
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

## Getting started

```
./perft "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" 7
```

![](<images/perft(7).png>)
