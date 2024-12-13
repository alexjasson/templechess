# templechess

The goal of this project is to find the the number of legal moves branching from any given chess position as fast as possible, with clear minimalistic code. For simplicity, we don't include threading, SIMD instructions or a hash table. With my Intel i5-10210U, it currently completes perft(7) in ~10.5s which equates to ~300M nodes per second.

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

![](<perft(7).png>)
