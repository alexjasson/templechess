CC = gcc
CFLAGS0 = -Wall -Wextra -Werror -Ofast -march=native -flto -funroll-loops
CFLAGS1 = -Wall -Wextra -Werror -Ofast -march=native -flto -funroll-loops -fprofile-generate
CFLAGS2 = -Wall -Wextra -Werror -Ofast -march=native -flto -funroll-loops -fprofile-use

.PHONY: all clean fast

all: perft

perft: perft.c BitBoard.c LookupTable.c ChessBoard.c
	$(CC) $(CFLAGS0) -o perft perft.c BitBoard.c LookupTable.c ChessBoard.c -lm

# Generate and use profile data
fast:
	@$(CC) $(CFLAGS1) -o perft perft.c BitBoard.c LookupTable.c ChessBoard.c -lm
	@./perft >/dev/null 2>&1
	$(CC) $(CFLAGS2) -o perft perft.c BitBoard.c LookupTable.c ChessBoard.c -lm
	@rm -f *.gcda *.gcno

clean:
	rm -f *.o perft