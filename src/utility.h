#ifndef UTILITY_H
#define UTILITY_H

#include <stdint.h>

typedef size_t Size;
typedef uint32_t U32;
typedef uint64_t U64;

void writeToFile(void *array, Size elementSize, Size numElements, char *filename);
void readFromFile(void *array, Size elementSize, Size numElements, char *filename);
bool isFileEmpty(char *filename);
U64 getRandomNumber();

#endif