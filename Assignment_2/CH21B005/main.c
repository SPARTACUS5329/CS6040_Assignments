#include "main.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define streq(str1, str2, n) (strncmp(str1, str2, n) == 0)

static int bitWidth;
static int numDroppedPackets = 0;

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
    int firstSwitchPort = isBenes ? i : circularLeftShift(i, 1, bitWidth);
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

  int upPrevPort = circularRightShift(upPort, 1, bitWidth);
  int downPrevPort = circularRightShift(downPort, 1, bitWidth);

  connectSwitches(switches, upPort, downPort, upPrevPort, downPrevPort, col,
                  currSwitch);
}

void connectDelta(int upPort, int downPort, int N, int M, switch_t ***switches,
                  int col, switch_t *currSwitch) { // N switches, M stages
  bool isFirstConnection = col == 1;
  bool isUpperHalf = upPort <= N / 2;
  int upPrevPort, downPrevPort;
  if (isFirstConnection && isUpperHalf) {
    upPrevPort = upPort;
    downPrevPort = (downPort + N) - 1; // offset by N and switch down -> up
  } else if (isFirstConnection) {      // lower half in first connection
    upPrevPort = (upPort - N) + 1;     // offset by N and switch up -> down
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

void connectBenes(int upPort, int downPort, int M, switch_t ***switches,
                  int col, switch_t *currSwitch) {
  bool isFirstConnection = col == 1;
  bool isLastConnection = col == M - 1;

  int upPrevPort, downPrevPort;
  if (isFirstConnection) {
    upPrevPort = circularLeftShift(upPort, 1, bitWidth);
    downPrevPort = circularLeftShift(downPort, 1, bitWidth);
  } else if (isLastConnection) {
    upPrevPort = circularRightShift(upPort, 1, bitWidth);
    downPrevPort = circularRightShift(downPort, 1, bitWidth);
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

int *selfRoutePackets(int A, int M, int *packets, input_t **inputs) {
  int *droppedPackets = (int *)malloc(A * sizeof(int));

  for (int i = 0; i < A; i++) {
    int packet = packets[i];
    input_t *input = inputs[i];
    switch_t *currSwitch = input->firstSwitch;
    int currPort = input->firstSwitchPort;

    for (int m = 0; m < M; m++) {
      int kBit = (packet >> (M - 1 - m)) & 1;
      if (kBit == 1) {
        if (isSwitchInContention(currSwitch, currPort, 1)) {
          droppedPackets[numDroppedPackets++] = packet;
          break;
        }

        currPort = currSwitch->downOutPort;
        currSwitch = currSwitch->downOut;
      } else {
        if (isSwitchInContention(currSwitch, currPort, 0)) {
          droppedPackets[numDroppedPackets++] = packet;
          break;
        }

        currPort = currSwitch->upOutPort;
        currSwitch = currSwitch->upOut;
      }
    }
  }

  return droppedPackets;
}

bool helperBenes(int m, int currPort, int destPort, switch_t *currSwitch) {
  if (m == 1) {
    if (currSwitch->upPort != destPort && currSwitch->downPort != destPort)
      return false;

    char requiredConfig = currPort == destPort ? 'T' : 'C';
    if (currSwitch->hasPacket && currSwitch->config != requiredConfig)
      return false;

    currSwitch->config = requiredConfig;
    currSwitch->hasPacket = true;
    return true;
  }

  bool isUpValid =
      !currSwitch->hasPacket ||
      (currSwitch->config == 'T' && currPort == currSwitch->upPort) ||
      (currSwitch->config == 'C' && currPort == currSwitch->downPort);
  bool isDownValid =
      !currSwitch->hasPacket ||
      (currSwitch->config == 'T' && currPort == currSwitch->downPort) ||
      (currSwitch->config == 'C' && currPort == currSwitch->upPort);

  if (isUpValid &&
      helperBenes(m - 1, currSwitch->upOutPort, destPort, currSwitch->upOut)) {
    currSwitch->config = currPort == currSwitch->upPort ? 'T' : 'C';
    currSwitch->hasPacket = true;
    return true;
  }
  if (isDownValid && helperBenes(m - 1, currSwitch->downOutPort, destPort,
                                 currSwitch->downOut)) {
    currSwitch->config = currPort == currSwitch->downPort ? 'T' : 'C';
    currSwitch->hasPacket = true;
    return true;
  }

  return false;
}

int *routeBenes(int A, int M, int *packets, input_t **inputs,
                switch_t ***switches) {
  int *droppedPackets = (int *)malloc(A * sizeof(int));

  for (int i = 0; i < A; i++) {
    int packet = packets[i];
    input_t *input = inputs[i];
    switch_t *currSwitch = input->firstSwitch;
    int currPort = input->firstSwitchPort;
    bool isRouted = helperBenes(M, currPort, packet, currSwitch);
    if (!isRouted)
      droppedPackets[numDroppedPackets++] = packet;
  }

  return droppedPackets;
}

bool isSwitchInContention(switch_t *currSwitch, int currPort, int dest) {
  if (!currSwitch->hasPacket) {
    currSwitch->hasPacket = true;
    currSwitch->config = currPort % 2 == dest ? 'T' : 'C';
    return false;
  }

  if (currSwitch->config == 'T') {
    if (currPort % 2 == dest) {
      currSwitch->config = 'T';
      return false;
    }
    return true;
  }

  if (currPort % 2 != dest) {
    currSwitch->config = 'C';
    return false;
  }

  return true;
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
      currSwitch->upPort = upPort;
      currSwitch->downPort = downPort;

      if (j == 0)
        goto addSwitch;

      if (streq(sw, "Omega", 5))
        connectOmega(upPort, downPort, M, switches, j, currSwitch);
      if (streq(sw, "Delta", 5))
        connectDelta(upPort, downPort, N, M, switches, j, currSwitch);
      if (streq(sw, "Benes", 5))
        connectBenes(upPort, downPort, M, switches, j, currSwitch);

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

  bitWidth = log2(N);

  switch_t ***switches = initializeSwitches(N / 2, M, sw);
  input_t **inputs = initializeInputs(N, M, switches, sw);
  int *droppedPackets;

  if (!isBenes)
    droppedPackets = selfRoutePackets(A, M, packets, inputs);
  else
    droppedPackets = routeBenes(A, M, packets, inputs, switches);

  for (int i = 0; i < N / 2; i++) {
    for (int j = 0; j < M; j++) {
      printf("%c", switches[i][j]->config);
      if (j != M - 1)
        printf(" ");
    }
    printf("\n");
  }

  if (numDroppedPackets > 0)
    printf("Dropped packets\n");
  for (int i = 0; i < numDroppedPackets; i++)
    printf("%d\n", droppedPackets[i]);
}
