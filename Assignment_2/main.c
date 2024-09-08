#include "main.h"
#include <math.h>
#include <stdbool.h>
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

input_t **initializeInputs(int N, int M, switch_t ***switches, char *sw) {
  bool isBenes = (bool)streq(sw, "Benes", 5);
  input_t **inputs = (input_t **)malloc(N * sizeof(input_t *));
  for (int i = 0; i < N; i++) {
    input_t *input = (input_t *)malloc(sizeof(switch_t));
    int firstSwitchPort = isBenes ? i : circularLeftShift(i, 1, M);
    int firstSwitchRow = firstSwitchPort / 2;
    int firstSwitchCol = 0;

    switch_t *firstSwitch = switches[firstSwitchRow][firstSwitchCol];
    input->firstSwitch = firstSwitch;
    input->firstSwitchPort = firstSwitchPort;

    input->id = i;
    inputs[i] = input;
  }
  return inputs;
}

void connectOmega(int upPort, int downPort, int M, switch_t ***switches,
                  int col, switch_t *currSwitch) {

  int upPrevPort = circularRightShift(upPort, 1, M);
  int downPrevPort = circularRightShift(downPort, 1, M);

  connectSwitches(switches, upPort, downPort, upPrevPort, downPrevPort, col,
                  currSwitch);
}

void connectDelta(int upPort, int downPort, int N, int M, switch_t ***switches,
                  int col, switch_t *currSwitch) {
  bool isFirstConnection = col == 1;
  bool isUpperHalf = upPort < N / 2;
  int upPrevPort, downPrevPort;
  if (isFirstConnection && isUpperHalf) {
    upPrevPort = upPort;
    downPrevPort =
        (downPort + N / 2) - 1;        // offset by N / 2 and switch down -> up
  } else if (isFirstConnection) {      // lower half in first connection
    upPrevPort = (upPort - N / 2) + 1; // offset by N / 2 and switch up -> down
    downPrevPort = downPort;
  } else if (upPort % 4 == 0) {
    upPrevPort = upPort;
    downPrevPort = downPort + 1;
  } else {
    upPrevPort = upPort - 1;
    downPrevPort = downPort;
  }
  connectSwitches(switches, upPort, downPort, upPrevPort, downPrevPort, col,
                  currSwitch);
}

void connectBenes(int upPort, int downPort, int N, int M, switch_t ***switches,
                  int col, switch_t *currSwitch) {
  bool isMeshConnection = col == 1 || col == M - 1;
  bool isUpperHalf = upPort < N / 2;
  int upPrevPort, downPrevPort;
  if (isMeshConnection && isUpperHalf) {
    upPrevPort = upPort;
    downPrevPort =
        (downPort + N / 2) - 1;        // offset by N / 2 and switch down -> up
  } else if (isMeshConnection) {       // lower half in first connection
    upPrevPort = (upPort - N / 2) + 1; // offset by N / 2 and switch up -> down
    downPrevPort = downPort;
  } else if (upPort % 4 == 0) {
    upPrevPort = upPort;
    downPrevPort = downPort + 1;
  } else {
    upPrevPort = upPort - 1;
    downPrevPort = downPort;
  }
  connectSwitches(switches, upPort, downPort, upPrevPort, downPrevPort, col,
                  currSwitch);
}

void connectSwitches(switch_t ***switches, int upPort, int downPort,
                     int upPrevPort, int downPrevPort, int col,
                     switch_t *currSwitch) {
  int upPrevSwitchRow = upPrevPort / 2;
  int downPrevSwitchRow = downPrevPort / 2;

  currSwitch->upIn = switches[upPrevSwitchRow][col - 1];
  currSwitch->upInPort = upPrevPort;
  currSwitch->downIn = switches[downPrevSwitchRow][col - 1];
  currSwitch->downInPort = downPrevPort;

  if (upPrevPort % 2) {
    switches[upPrevSwitchRow][col - 1]->downOut = currSwitch;
    switches[upPrevSwitchRow][col - 1]->downOutPort = upPort;
  } else {
    switches[upPrevSwitchRow][col - 1]->upOut = currSwitch;
    switches[upPrevSwitchRow][col - 1]->upOutPort = upPort;
  }

  if (downPrevPort % 2) {
    switches[downPrevSwitchRow][col - 1]->downOut = currSwitch;
    switches[downPrevSwitchRow][col - 1]->downOutPort = downPort;
  } else {
    switches[downPrevSwitchRow][col - 1]->upOut = currSwitch;
    switches[downPrevSwitchRow][col - 1]->upOutPort = downPort;
  }
}

void selfRoutePackets(int N, int A, int M, switch_t ***switches, int *packets,
                      input_t **inputs) {
  for (int i = 0; i < A; i++) {
    int packet = packets[i];
    input_t *input = inputs[i];
    switch_t *currSwitch = input->firstSwitch;
    int currPort = input->firstSwitchPort;
    for (int m = 0; m < M; m++) {
      int kBit = (packet >> (M - 1 - m)) & 1;
      switch (kBit) {
      case 1:
        currSwitch->config = isSwitchInContention(currSwitch, currPort, 1);
        currPort = currSwitch->downOutPort;
        currSwitch = currSwitch->downOut;
        break;
      case 0:
        currSwitch->config = isSwitchInContention(currSwitch, currPort, 0);
        currPort = currSwitch->upOutPort;
        currSwitch = currSwitch->upOut;
        break;
      }
    }
  }
}

char isSwitchInContention(switch_t *currSwitch, int currPort, int dest) {
  if (!currSwitch->hasPacket) {
    currSwitch->hasPacket = true;
    return currPort % 2 == dest ? 'T' : 'C';
  }
  if (currSwitch->config == 'T') {
    if (currPort % 2 == dest)
      return 'T';
    error("Switch contention");
  }
  if (currPort % 2 != dest)
    return 'C';
  error("Switch contention");
  return '\0'; // because the linter is not happy
}

switch_t ***initializeSwitches(int N, int M,
                               char *sw) { // M stages and N switches
  switch_t ***switches = (switch_t ***)malloc(N * sizeof(switch_t **));
  int currID = 0;
  for (int j = 0; j < M; j++) {
    for (int i = 0; i < N; i++) {
      if (j == 0)
        switches[i] = (switch_t **)malloc(M * sizeof(switch_t *));
      switch_t *currSwitch = (switch_t *)malloc(sizeof(switch_t));
      currSwitch->id = currID++;
      currSwitch->row = i;
      currSwitch->col = j;
      currSwitch->config = 'T';
      uint32_t upPort = 2 * i;
      uint32_t downPort = upPort + 1;

      if (j == 0)
        goto addSwitch;

      if (streq(sw, "Omega", 5))
        connectOmega(upPort, downPort, M, switches, j, currSwitch);
      if (streq(sw, "Delta", 5))
        connectDelta(upPort, downPort, N, M, switches, j, currSwitch);
      if (streq(sw, "Benes", 5))
        connectOmega(upPort, downPort, M, switches, j, currSwitch);

    addSwitch:
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
  int *packets = readInputFile(inputFile, &N, &A);
  int M;
  bool isBenes = streq(sw, "Benes", 5);

  if (isBenes)
    M = 2 * log2(N) - 1;
  else
    M = log2(N);

  switch_t ***switches = initializeSwitches(N / 2, M, sw);
  input_t **inputs = initializeInputs(N, M, switches, sw);
  if (!isBenes)
    selfRoutePackets(N, A, M, switches, packets, inputs);
  for (int i = 0; i < N / 2; i++) {
    for (int j = 0; j < M; j++) {
      switch_t *currSwitch = switches[i][j];
      printf("%c ", currSwitch->config);
    }
    printf("\n");
  }
}
