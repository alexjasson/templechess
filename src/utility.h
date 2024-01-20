#ifndef UTILITY_H
#define UTILITY_H

void writeToFile(void *array, size_t elementSize, size_t numElements, char *filename);
void readFromFile(void *array, size_t elementSize, size_t numElements, char *filename);
bool isFileEmpty(char *filename);
uint64_t getRandomNumber(pthread_mutex_t *lock);

#endif