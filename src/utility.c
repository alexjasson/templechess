#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "utility.h"

#define SEED 1804289383

static U32 xorshift();

void writeToFile(void *array, Size elementSize, Size numElements, char *filename) {
  FILE *fp = fopen(filename, "wb");
  if (fp == NULL) {
    fprintf(stderr, "Failed to open '%s' for writing\n", filename);
    exit(EXIT_FAILURE);
  }

  Size written = fwrite(array, elementSize, numElements, fp);
  if (written != numElements) {
    fprintf(stderr, "Failed to write all elements to '%s'\n", filename);
    exit(EXIT_FAILURE);
  }

  fclose(fp);
}

void readFromFile(void *array, Size elementSize, Size numElements, char *filename) {
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    fprintf(stderr, "Failed to open '%s' for reading\n", filename);
    exit(EXIT_FAILURE);
  }

  Size read = fread(array, elementSize, numElements, fp);
  if (read != numElements) {
    fprintf(stderr, "Failed to read all elements from '%s'\n", filename);
    exit(EXIT_FAILURE);
  }

  fclose(fp);
}

bool isFileEmpty(char *filename) {
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) return true;

  fseek(fp, 0, SEEK_END);
  int size = ftell(fp);
  fclose(fp);

  return size == 0;
}

// generate random U64 number
U64 getRandomNumber() {
  U64 u1, u2, u3, u4;

  u1 = (U64)(xorshift()) & 0xFFFF;
  u2 = (U64)(xorshift()) & 0xFFFF;
  u3 = (U64)(xorshift()) & 0xFFFF;
  u4 = (U64)(xorshift()) & 0xFFFF;

  return u1 | (u2 << 16) | (u3 << 32) | (u4 << 48);
}

// 32-bit number pseudo random generator - not thread safe
static U32 xorshift() {
  static U32 state = 0;
  static bool seeded = false;

  // Seed only once
  if (!seeded) {
    state = SEED;
    seeded = true;
  }

  U32 x = state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;

  return state = x;
}
