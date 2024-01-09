CC = gcc
CFLAGS = -Wall -Wextra -Werror
TARGET = go

# List all your supporting .c files here. Do NOT include .h files in this list.
# Example: SUPPORTING_FILES = hello.c world.c
# SUPPORTING_FILES = ...

.PHONY: all
all: $(TARGET)

$(TARGET): test.c BitBoard.c LookupTable.c ChessBoard.c utility.c
	$(CC) $(CFLAGS) -o $(TARGET) test.c BitBoard.c LookupTable.c ChessBoard.c utility.c -lm

.PHONY: clean
clean:
	rm -f *.o $(TARGET)