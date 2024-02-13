CC = gcc
CFLAGS = -Wall -Wextra -Werror -Ofast

.PHONY: all
all: perft
perft: perft.c BitBoard.c LookupTable.c ChessBoard.c utility.c
	$(CC) $(CFLAGS) -o perft perft.c BitBoard.c LookupTable.c ChessBoard.c utility.c -lm

.PHONY: clean
clean:
	rm -f *.o perft