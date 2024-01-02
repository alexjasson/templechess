#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utility.h"

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