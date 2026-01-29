# Detect compiler
ifneq ($(shell which gcc 2>/dev/null),)
    CC := gcc
else
    $(error Compiler not found! Please install gcc)
endif

# Detect BMI2 support
ifeq ($(shell grep -q bmi2 /proc/cpuinfo && echo yes),yes)
    BMI2 := -mbmi2
else
    BMI2 :=
    $(info BMI2 instructions not supported, compiling without it)
endif

CFLAGS0 = -Ofast -march=native -flto -fprofile-generate $(BMI2)
CFLAGS1 = -Ofast -march=native -flto -fprofile-use $(BMI2)
CFLAGS2 = -fsanitize=undefined -Wall -Wextra -Werror -pedantic -Ofast -march=native -flto $(BMI2)

# Position/depth to be used for profiling
BOARD = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
DEPTH = 5

# Targets
.PHONY: all clean

all: perft test

perft:
	@$(CC) $(CFLAGS0) -o perft src/perft.c src/BitBoard.c src/LookupTable.c src/ChessBoard.c src/MoveSet.c -lm
	@./perft $(BOARD) $(DEPTH) >/dev/null 2>&1
	$(CC) $(CFLAGS1) -o perft src/perft.c src/BitBoard.c src/LookupTable.c src/ChessBoard.c src/MoveSet.c -lm
	@rm -f *.gcda *.gcno

test:
	$(CC) $(CFLAGS2) -o test src/test.c src/BitBoard.c src/LookupTable.c src/ChessBoard.c src/MoveSet.c -lm

clean:
	rm -f *.o perft test


