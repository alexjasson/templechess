CC = gcc
CFLAGS0 = -Wall -Wextra -Werror -Ofast -march=native -flto
CFLAGS1 = -Wall -Wextra -Werror -Ofast -march=native -flto -fprofile-generate
CFLAGS2 = -Wall -Wextra -Werror -Ofast -march=native -flto -fprofile-use

.PHONY: all clean fast

all: perft

perft: perft.c BitBoard.c LookupTable.c ChessBoard.c utility.c
	$(CC) $(CFLAGS0) -o perft perft.c BitBoard.c LookupTable.c ChessBoard.c utility.c -lm

# Generate and use profile data
fast:
	@$(CC) $(CFLAGS1) -o perft perft.c BitBoard.c LookupTable.c ChessBoard.c utility.c -lm
	@./perft >/dev/null 2>&1
	$(CC) $(CFLAGS2) -o perft perft.c BitBoard.c LookupTable.c ChessBoard.c utility.c -lm
	@rm -f *.gcda *.gcno

clean:
	rm -f *.o perft