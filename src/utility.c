#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "utility.h"

static uint32_t xorshift();

void writeToFile(void *array, size_t elementSize, size_t numElements, char *filename, long offset) {
  FILE *fp = fopen(filename, "r+b");
  if (fp == NULL) {
    fprintf(stderr, "Failed to open '%s' for writing\n", filename);
    exit(EXIT_FAILURE);
  }

  fseek(fp, offset, SEEK_SET);

  size_t written = fwrite(array, elementSize, numElements, fp);
  if (written != numElements) {
    fprintf(stderr, "Failed to write all elements to '%s'\n", filename);
    exit(EXIT_FAILURE);
  }

  fclose(fp);
}

void readFromFile(void *array, size_t elementSize, size_t numElements, char *filename, long offset) {
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    fprintf(stderr, "Failed to open '%s' for reading\n", filename);
    exit(EXIT_FAILURE);
  }

  fseek(fp, offset, SEEK_SET);

  size_t read = fread(array, elementSize, numElements, fp);
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

// 64-bit PRNG - thread safe
uint64_t getRandomNumber() {
  uint64_t u1, u2, u3, u4;

  u1 = (uint64_t)(xorshift()) & 0xFFFF;
  u2 = (uint64_t)(xorshift()) & 0xFFFF;
  u3 = (uint64_t)(xorshift()) & 0xFFFF;
  u4 = (uint64_t)(xorshift()) & 0xFFFF;

  return u1 | (u2 << 16) | (u3 << 32) | (u4 << 48);
}

static uint32_t xorshift() {
  static uint32_t state = 0;
  static bool seeded = false;

  // Seed only once
  if (!seeded) {
    state = (uint32_t)time(NULL);
    seeded = true;
  }

  uint32_t x = state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;

  state = x;
  return x;
}
