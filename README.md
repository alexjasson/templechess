# templechess

This is an open source chess move generator written in C. The goal of the project is to count the legal number of chess positions (nodes) reachable from any given position as fast as possible on a CPU. It's currently single threaded and doesn't have a transposition table. With my Intel i5-8400 (Max 4Ghz), it completes perft(7) in ~1.6s which equates to ~2 billion nodes per second.

This repository introduces depth-two counting, a specialized algorithm for computing chess perft. For a detailed report on it, see [Report.pdf](Report.pdf).

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
