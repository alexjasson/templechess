#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>

#include "utility.h"

static uint32_t xorshift(pthread_mutex_t *lock);

void writeElementToFile(void *element, size_t elementSize, int position, char *filename) {
  FILE *fp = fopen(filename, "r+b"); // Open for reading and writing; file must exist
  if (fp == NULL) {
    fprintf(stderr, "Failed to open '%s' for writing\n", filename);
    exit(EXIT_FAILURE);
  }

  fseek(fp, position * elementSize, SEEK_SET);

  size_t written = fwrite(element, elementSize, 1, fp);
  if (written != 1) {
    fprintf(stderr, "Failed to write the element to '%s'\n", filename);
    exit(EXIT_FAILURE);
  }

  fclose(fp);
}

bool readElementFromFile(void *element, size_t elementSize, int position, char *filename) {
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) return false;

  fseek(fp, position * elementSize, SEEK_SET);

  size_t read = fread(element, elementSize, 1, fp);
  fclose(fp);
  return (read != 1) ? false : true;
}

bool isFileEmpty(char *filename) {
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) return true;

  fseek(fp, 0, SEEK_END);
  int size = ftell(fp);
  fclose(fp);

  return size == 0;
}

// generate pseudo random 64 bit number - thread safe
uint64_t getRandomNumber(pthread_mutex_t *lock) {
  uint64_t u1, u2, u3, u4;

  u1 = (uint64_t)(xorshift(lock)) & 0xFFFF;
  u2 = (uint64_t)(xorshift(lock)) & 0xFFFF;
  u3 = (uint64_t)(xorshift(lock)) & 0xFFFF;
  u4 = (uint64_t)(xorshift(lock)) & 0xFFFF;

  return u1 | (u2 << 16) | (u3 << 32) | (u4 << 48);
}

// 32-bit number pseudo random generator - thread safe
static uint32_t xorshift(pthread_mutex_t *lock) {
  static uint32_t state = 0;
  static bool seeded = false;

  pthread_mutex_lock(lock);
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
  pthread_mutex_unlock(lock);
  return x;
}
