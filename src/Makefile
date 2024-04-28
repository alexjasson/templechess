CC = gcc
CFLAGS0 = -Wall -Wextra -Werror -Ofast -march=native -flto -funroll-loops
CFLAGS1 = -Wall -Wextra -Werror -Ofast -march=native -flto -funroll-loops -fprofile-generate
CFLAGS2 = -Wall -Wextra -Werror -Ofast -march=native -flto -funroll-loops -fprofile-use

# Position/depth to be used for profiling
BOARD = "k1q5/8/1K6/8/2N5/5n2/1R6/B7 b - - 0 1"
DEPTH = 5

.PHONY: all clean fast

all: perft

perft: perft.c BitBoard.c LookupTable.c ChessBoard.c
	$(CC) $(CFLAGS0) -o perft perft.c BitBoard.c LookupTable.c ChessBoard.c -lm

# Generate and use profile data
fast:
	@$(CC) $(CFLAGS1) -o perft perft.c BitBoard.c LookupTable.c ChessBoard.c -lm
	@./perft $(BOARD) $(DEPTH) >/dev/null 2>&1
	$(CC) $(CFLAGS2) -o perft perft.c BitBoard.c LookupTable.c ChessBoard.c -lm
	@rm -f *.gcda *.gcno

clean:
	rm -f *.o perft
