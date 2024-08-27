# Detect compiler
ifneq ($(shell which gcc 2>/dev/null),)
    CC := gcc
else
    $(error Compiler not found! Please install gcc)
endif

CFLAGS0 = -Ofast -march=native -flto -fprofile-generate
CFLAGS1 = -Ofast -march=native -flto -fprofile-use
CFLAGS2 = -fsanitize=undefined -Wall -Wextra -Werror -Ofast -march=native -flto

# Position/depth to be used for profiling
BOARD = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
DEPTH = 5

# Targets
.PHONY: all clean

all: perft test

perft:
	@$(CC) $(CFLAGS0) -o perft perft.c BitBoard.c LookupTable.c ChessBoard.c Branch.c -lm
	@./perft $(BOARD) $(DEPTH) >/dev/null 2>&1
	$(CC) $(CFLAGS1) -o perft perft.c BitBoard.c LookupTable.c ChessBoard.c Branch.c -lm
	@rm -f *.gcda *.gcno

test:
	$(CC) $(CFLAGS2) -o test test.c BitBoard.c LookupTable.c ChessBoard.c Branch.c -lm

clean:
	rm -f *.o perft test


