#ifndef UTILITY_H
#define UTILITY_H

typedef size_t Size;

void writeToFile(void *array, Size elementSize, Size numElements, char *filename);
void readFromFile(void *array, Size elementSize, Size numElements, char *filename);
bool isFileEmpty(char *filename);

#endif