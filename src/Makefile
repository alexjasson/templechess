# Detect operating system
OS := $(shell uname -s)

# Detect compiler
ifneq ($(shell which clang 2>/dev/null),)
    CC := clang
else ifneq ($(shell which gcc 2>/dev/null),)
    CC := gcc
else
    $(error Compiler not found! Please install gcc or clang)
endif

CFLAGS_COMMON = -Wall -Wextra -Werror -Ofast -march=native -flto
LDFLAGS_COMMON = -lm

# OS-specific settings
CFLAGS_PROFILE_GEN = -fprofile-generate
CFLAGS_PROFILE_USE = -fprofile-use
CFLAGS_SANITIZE = -fsanitize=undefined

ifeq ($(OS), Windows_NT)
    CFLAGS_SANITIZE = -fsanitize=address
    LDFLAGS_COMMON += -lws2_32
endif

# Position/depth to be used for profiling
BOARD = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
DEPTH = 5

# Targets
.PHONY: all clean

all: perft testChessBoard

perft:
	@$(CC) $(CFLAGS_COMMON) $(CFLAGS_PROFILE_GEN) -o perft perft.c BitBoard.c LookupTable.c ChessBoard.c $(LDFLAGS_COMMON)
	@./perft $(BOARD) $(DEPTH) >/dev/null 2>&1
	$(CC) $(CFLAGS_COMMON) $(CFLAGS_PROFILE_USE) -o perft perft.c BitBoard.c LookupTable.c ChessBoard.c $(LDFLAGS_COMMON)
	@rm -f *.gcda *.gcno

testChessBoard:
	$(CC) $(CFLAGS_COMMON) $(CFLAGS_SANITIZE) -o testChessBoard testChessBoard.c BitBoard.c LookupTable.c ChessBoard.c $(LDFLAGS_COMMON)

clean:
	rm -f *.o perft testChessBoard


