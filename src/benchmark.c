/*
 * TempleChess v2
 * © 2026 Alex Jasson
 */

#include "BitBoard.h"
#include "LookupTable.h"
#include "ChessBoard.h"
#include "MoveSet.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SEED 1
#define DEPTH 6
#define ROUNDS 5
#define BUCKETS 8
#define POSITIONS 1000
#define OUTPUT "data/benchmark.out"

typedef struct
{
  long nodes;
  double with;    // seconds, depth-two counting enabled
  double without; // seconds, depth-two counting disabled
} Result;

static long treeSearch(LookupTable l, ChessBoard *cb, int depth, int with)
{
  if (depth == 1)
    return ChessBoardCount(l, cb);

  MoveSet ms = MoveSetNew();
  MoveSetFill(l, cb, &ms);

  long nodes = 0;
  if (with && depth == 2)
    nodes += MoveSetMultiply(l, &ms);

  while (!MoveSetIsEmpty(&ms))
  {
    Move m = MoveSetPop(&ms);
    ChessBoardPlayMove(cb, m);
    nodes += treeSearch(l, cb, depth - 1, with);
    ChessBoardUndoMove(cb, m);
  }

  return nodes;
}

static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (double)t.tv_sec + (double)t.tv_nsec * 1e-9; }
static int compareDouble(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return (x > y) - (x < y); }

static double median(double *values, int n)
{
  qsort(values, (size_t)n, sizeof(double), compareDouble);
  return (n & 1) ? values[n / 2] : (values[n / 2 - 1] + values[n / 2]) / 2;
}

static void printSpeedups(const char *name, double *speedups, int n)
{
  if (n == 0)
  {
    printf("%-12s %3d positions\n", name, 0);
    return;
  }

  double logSum = 0;
  for (int i = 0; i < n; i++)
    logSum += log(speedups[i]);
  double middle = median(speedups, n);

  printf("%-12s %3d positions: %.2fx geometric mean, %.2fx median, "
         "%.2fx min, %.2fx max\n",
         name, n, exp(logSum / n), middle, speedups[0], speedups[n - 1]);
}

int main(int argc, char **argv)
{
  if (argc > 4)
  {
    fprintf(stderr, "Usage: %s <depth> <positions> <seed>\n", argv[0]);
    return EXIT_FAILURE;
  }

  int depth = (argc > 1) ? atoi(argv[1]) : DEPTH;
  int positions = (argc > 2) ? atoi(argv[2]) : POSITIONS;
  unsigned seed = (argc > 3) ? (unsigned)strtoul(argv[3], NULL, 10) : SEED;

  if (depth < 2)
  {
    fprintf(stderr, "Depth must be at least 2\n");
    return EXIT_FAILURE;
  }
  if (positions < 1)
  {
    fprintf(stderr, "Positions must be positive\n");
    return EXIT_FAILURE;
  }

  FILE *out = fopen(OUTPUT, "w");
  if (!out)
  {
    perror(OUTPUT);
    return EXIT_FAILURE;
  }

  LookupTable l = LookupTableNew();

  ChessBoard *sample = malloc((size_t)positions * sizeof(ChessBoard));
  double *speedups = malloc((size_t)positions * sizeof(double));
  double *bucketSpeedups = malloc((size_t)BUCKETS * (size_t)positions * sizeof(double));
  if (!sample || !speedups || !bucketSpeedups)
  {
    fprintf(stderr, "Out of memory\n");
    return EXIT_FAILURE;
  }
  int bucketCounts[BUCKETS] = {0};

  for (int i = 0; i < positions; i++)
    sample[i] = ChessBoardRandom(l, (seed << 16) + (unsigned)i);

  fprintf(out, "index,pieces,nodes,with_ms,without_ms,speedup,fen\n");

  long totalNodes = 0;
  double totalWith = 0, totalWithout = 0;

  for (int i = 0; i < positions; i++)
  {
    ChessBoard cb = sample[i];
    Result r = {0, 0, 0};
    long nodes[2];            // indexed by with
    double times[2 * ROUNDS]; // [with][round]

    for (int k = 0; k < ROUNDS; k++)
    {
      for (int turn = 0; turn < 2; turn++)
      {
        int with = (turn == 0) == !((i + k) & 1);
        double started = now();
        nodes[with] = treeSearch(l, &cb, depth, with);
        times[with * ROUNDS + k] = now() - started;
      }

      if (nodes[1] != nodes[0])
      {
        fprintf(stderr, "\nDisagreement at depth %d: with %ld, without %ld: %s\n",
                depth, nodes[1], nodes[0], ChessBoardToFEN(&cb));
        return EXIT_FAILURE;
      }
    }

    r.nodes = nodes[1];
    r.with = median(times + ROUNDS, ROUNDS);
    r.without = median(times, ROUNDS);

    speedups[i] = r.without / r.with;
    totalNodes += r.nodes;
    totalWith += r.with;
    totalWithout += r.without;

    int pieces = BitBoardCount(ChessBoardAll(&cb));
    int b = (pieces - 1) / 4;
    bucketSpeedups[b * positions + bucketCounts[b]++] = speedups[i];

    fprintf(out, "%d,%d,%ld,%.4f,%.4f,%.4f,%s\n",
            i + 1, pieces, r.nodes, r.with * 1e3, r.without * 1e3,
            speedups[i], ChessBoardToFEN(&cb));
    fflush(out);
    fprintf(stderr, "\rposition %d/%d", i + 1, positions);
  }
  fprintf(stderr, "\n");
  fclose(out);

  printf("nodes per second: %.2f M with, %.2f M without\n",
        (double)totalNodes / totalWith / 1e6, (double)totalNodes / totalWithout / 1e6);
  printSpeedups("all", speedups, positions);
  for (int b = 0; b < BUCKETS; b++)
  {
    char name[16];
    snprintf(name, sizeof name, "%d-%d pieces", b ? 4 * b + 1 : 2, 4 * b + 4);
    printSpeedups(name, bucketSpeedups + b * positions, bucketCounts[b]);
  }
  printf("Results saved to %s\n", OUTPUT);

  free(sample);
  free(speedups);
  free(bucketSpeedups);
  LookupTableFree(l);
  return EXIT_SUCCESS;
}
