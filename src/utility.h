#ifndef UTILITY_H
#define UTILITY_H

#include <stdint.h>

typedef uint32_t U32;
typedef uint64_t U64;

void writeElementToFile(void *element, size_t elementSize, int position, char *filename);
bool readElementFromFile(void *element, size_t elementSize, int position, char *filename);
bool isFileEmpty(char *filename);
U64 getRandomNumber(pthread_mutex_t *lock);

#endif