CC = gcc
CFLAGS0 = -Wall -Wextra -Werror -Ofast
CLAGS1 = -Ofast

.PHONY: all
all: perft test

perft: perft.c BitBoard.c LookupTable.c ChessBoard.c utility.c
	$(CC) $(CFLAGS) -o perft perft.c BitBoard.c LookupTable.c ChessBoard.c utility.c -lm

test: test.c BitBoard.c LookupTable.c ChessBoard.c utility.c
	$(CC) $(CFLAGS) -o test test.c BitBoard.c LookupTable.c ChessBoard.c utility.c -lm

.PHONY: clean
clean:
	rm -f *.o perft test