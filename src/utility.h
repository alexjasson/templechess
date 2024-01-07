#ifndef UTILITY_H
#define UTILITY_H

void writeElementToFile(void *element, size_t elementSize, int position, char *filename);
bool readElementFromFile(void *element, size_t elementSize, int position, char *filename);
bool isFileEmpty(char *filename);
uint64_t getRandomNumber(pthread_mutex_t *lock);

#endif