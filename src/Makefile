CC = gcc
CFLAGS0 = -Wall -Wextra -Werror -Ofast -march=native -flto -funroll-loops -fprofile-generate
CFLAGS1 = -Wall -Wextra -Werror -Ofast -march=native -flto -funroll-loops -fprofile-use

# Position/depth to be used for profiling
BOARD = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1"
DEPTH = 5

.PHONY: all clean

all: perft

perft:
	@$(CC) $(CFLAGS0) -o perft perft.c BitBoard.c LookupTable.c ChessBoard.c -lm
	@./perft $(BOARD) $(DEPTH) >/dev/null 2>&1
	$(CC) $(CFLAGS1) -o perft perft.c BitBoard.c LookupTable.c ChessBoard.c -lm
	@rm -f *.gcda *.gcno

clean:
	rm -f *.o perft
