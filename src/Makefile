CC = gcc
CFLAGS = -Wall -Wextra -Werror
TARGET = myprogram

# List all your supporting .c files here. Do NOT include .h files in this list.
# Example: SUPPORTING_FILES = hello.c world.c
# SUPPORTING_FILES = ...

.PHONY: all
all: $(TARGET)

$(TARGET): test.c BitBoard.c
	$(CC) $(CFLAGS) -o $(TARGET) test.c BitBoard.c -lm

.PHONY: clean
clean:
	rm -f *.o $(TARGET)