#ifndef UTILITY_H
#define UTILITY_H

#include <stdint.h>

typedef size_t Size;
typedef uint32_t U32;
typedef uint64_t U64;

void writeElementToFile(void *element, Size elementSize, Size position, char *filename);
bool readElementFromFile(void *element, Size elementSize, Size position, char *filename);
bool isFileEmpty(char *filename);
U64 getRandomNumber();

#endif