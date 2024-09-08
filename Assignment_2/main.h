#pragma once
#include <stdbool.h>
#include <stdlib.h>

typedef struct Switch {
  char config;
  int id;
  int row;
  int col;
  bool hasPacket;
  int upInPort;
  int upOutPort;
  int downInPort;
  int downOutPort;
  struct Switch *upIn;
  struct Switch *upOut;
  struct Switch *downIn;
  struct Switch *downOut;
} switch_t;

typedef struct Input {
  int id;
  char position;
  int firstSwitchPort;
  struct Switch *firstSwitch;
} input_t;

void error(char *message);
uint32_t circularLeftShift(uint32_t num, int shift, int bitWidth);
uint32_t circularRightShift(uint32_t num, int shift, int bitWidth);
int *readInputFile(const char *filename, int *N, int *A);
switch_t ***initializeSwitches(int N, int M, char *sw);
input_t **initializeInputs(int N, int M, switch_t ***swiches, char *sw);
void connectOmega(int upPort, int downPort, int M, switch_t ***switches,
                  int col, switch_t *currSwitch);
void connectDelta(int upPort, int downPort, int N, int M, switch_t ***switches,
                  int col, switch_t *currSwitch);
void connectSwitches(switch_t ***switches, int upPort, int downPort,
                     int upPrevPort, int downPrevPort, int col,
                     switch_t *currSwitch);
void selfRoutePackets(int N, int A, int M, switch_t ***switches, int *packets,
                      input_t **inputs);
char isSwitchInContention(switch_t *currSwitch, int currPort, int dest);
