#ifndef UTILITY_H
#define UTILITY_H

void writeToFile(void *array, size_t elementSize, size_t numElements, char *filename, long offset);
void readFromFile(void *array, size_t elementSize, size_t numElements, char *filename, long offset);
bool isFileEmpty(char *filename);
uint64_t getRandomNumber();

#endif