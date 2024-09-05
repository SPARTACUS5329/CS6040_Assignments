#pragma once
#include <stdlib.h>

typedef struct Switch {
  char config;
  int id;
  int row;
  int col;
  struct Switch *upIn;
  struct Switch *upOut;
  struct Switch *downIn;
  struct Switch *downOut;
} switch_t;

typedef struct Input {
  int id;
  char position;
  struct Switch *firstSwitch;
} input_t;

void error(char *message);
uint32_t circularLeftShift(uint32_t num, int shift, int bitWidth);
uint32_t circularRightShift(uint32_t num, int shift, int bitWidth);
int *readInputFile(const char *filename, int *N, int *A);
switch_t ***initializeSwitches(int N, int M, char *sw);
input_t **initializeInputs(int N, int M, switch_t ***swiches);
