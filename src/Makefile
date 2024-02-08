CC = gcc
CFLAGS = -Wall -Wextra -Werror -Ofast
TARGET = perft

.PHONY: all
all: $(TARGET)

$(TARGET): test.c BitBoard.c LookupTable.c ChessBoard.c utility.c
	$(CC) $(CFLAGS) -o $(TARGET) test.c BitBoard.c LookupTable.c ChessBoard.c utility.c -lm

.PHONY: clean
clean:
	rm -f *.o $(TARGET)