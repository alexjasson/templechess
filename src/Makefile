CC = gcc
CFLAGS0 = -Wall -Wextra -Werror -Ofast -march=native -flto -funroll-loops
CFLAGS1 = -Wall -Wextra -Werror -Ofast -march=native -flto -funroll-loops -fprofile-generate
CFLAGS2 = -Wall -Wextra -Werror -Ofast -march=native -flto -funroll-loops -fprofile-use

# Position/depth to be used for profiling
BOARD = "8/rnbqkbnr/pppppppp/8/8/PPPPPPPP/RNBQKBNR/8 w - - 0 1"
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
