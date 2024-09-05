#include "main.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define streq(str1, str2, n) (strncmp(str1, str2, n) == 0)

int *readInputFile(const char *filename, int *N, int *A) {
  FILE *file = fopen(filename, "r");
  if (file == NULL)
    error("Error: Unable to open topology file %s\n");

  fscanf(file, "%d", N);
  fscanf(file, "%d", A);

  int *requests = malloc((*N) * sizeof(int));

  for (int i = 0; i < *A; i++)
    fscanf(file, "%d", &requests[i]);

  fclose(file);
  return requests;
}

input_t **initializeInputs(int N, int M, switch_t ***switches) {
  input_t **inputs = (input_t **)malloc(N * sizeof(input_t *));
  for (int i = 0; i < N; i++) {
    input_t *input = (input_t *)malloc(sizeof(switch_t));
    int firstSwitchPort = circularLeftShift(i, 1, M);
    int firstSwitchRow = firstSwitchPort / 2;
    int firstSwitchCol = 0;

    switch_t *firstSwitch = switches[firstSwitchRow][firstSwitchCol];
    input->firstSwitch = firstSwitch;

    input->id = i;
    inputs[i] = input;
  }
  return inputs;
}

switch_t ***initializeSwitches(int N, int M,
                               char *sw) { // M stages and N switches
  switch_t ***switches = (switch_t ***)malloc(N * sizeof(switch_t **));
  int currID = 0;
  for (int j = 0; j < M; j++) {
    for (int i = 0; i < N; i++) {
      if (j == 0)
        switches[i] = (switch_t **)malloc(sizeof(switch_t *));
      switch_t *currSwitch = (switch_t *)malloc(sizeof(switch_t));
      currSwitch->id = currID++;
      currSwitch->row = i;
      currSwitch->col = j;
      currSwitch->config = 'T';
      uint32_t upPort = 2 * i;
      uint32_t downPort = upPort + 1;

      if (j != 0) {
        int upPrevPort = circularRightShift(upPort, 1, M);
        int downPrevPort = circularRightShift(downPort, 1, M);
        int upPrevSwitchRow = upPrevPort / 2;
        int downPrevSwitchRow = downPrevPort / 2;

        currSwitch->upIn = switches[upPrevSwitchRow][j - 1];
        currSwitch->downIn = switches[downPrevSwitchRow][j - 1];

        if (upPrevSwitchRow % 2)
          switches[upPrevSwitchRow][j - 1]->downOut = currSwitch;
        else
          switches[upPrevSwitchRow][j - 1]->upOut = currSwitch;

        if (downPrevSwitchRow % 2)
          switches[downPrevSwitchRow][j - 1]->downOut = currSwitch;
        else
          switches[downPrevSwitchRow][j - 1]->upOut = currSwitch;
      }
      switches[i][j] = currSwitch;
    }
  }
  return switches;
}

uint32_t circularLeftShift(uint32_t num, int shift, int bitWidth) {
  return ((num << shift) | (num >> (bitWidth - shift))) &
         ((uint32_t)pow(2, bitWidth) - 1);
}

uint32_t circularRightShift(uint32_t num, int shift, int bitWidth) {
  return ((num >> shift) | (num << (bitWidth - shift))) &
         ((uint32_t)pow(2, bitWidth) - 1);
}

void error(char *message) {
  perror(message);
  exit(1);
}

int main(int argc, char *argv[]) {
  if (argc != 5)
    error("Not enough input arguments");

  char *inputFile = NULL;
  char *sw = NULL;

  for (int i = 1; i < argc; i++) {
    if (streq(argv[i], "-in", 3))
      inputFile = argv[++i];
    else if (streq(argv[i], "-sw", 3)) {
      sw = argv[++i];
      if (!streq(sw, "Omega", 5) && !streq(sw, "Delta", 5) &&
          !streq(sw, "Benes", 5))
        error("Error: Invalid value for -flag. Expected 'Omega' or 'Delta' or "
              "'Benes'.\n");
    } else
      error("Error: Unknown argument\n");
  }

  int N, A;
  int *requests = readInputFile(inputFile, &N, &A);
  int M = log2(N);
  switch_t ***switches = initializeSwitches(N / 2, M, sw);
  input_t **inputs = initializeInputs(N, M, switches);
}
