#include "main.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

conn_list_t *readInputFile(const char *filename, param_t *params) {
  FILE *file = fopen(filename, "r");
  if (file == NULL)
    error("Failed to open input file");

  fscanf(file, "N=%d T=%d C=%d B=%d", &params->N, &params->T, &params->C,
         &params->B);

  conn_list_t *connList = (conn_list_t *)malloc(sizeof(conn_list_t));
  connList->connCount = params->N;
  connList->conns = (conn_t **)malloc(params->N * sizeof(conn_t *));

  for (int i = 0; i < params->N; i++) {
    conn_t *conn = (conn_t *)malloc(sizeof(conn_t));

    fscanf(file, "%d %d %d %d %lf %lf", &conn->p, &conn->lMin, &conn->lMax,
           &conn->w, &conn->tStart, &conn->tEnd);

    connList->conns[i] = conn;
  }

  fclose(file);
  return connList;
}

double generateInterArrivalTimes(double mu) {
  double uniformRandom = (double)rand() / RAND_MAX;
  return -log(1 - uniformRandom) / mu;
}

packet_t *generatePacket(conn_t *conn, double prevTime,
                         double interArrivalTime) {
  int packetLength = getRandomNumber(conn->lMin, conn->lMax);
  packet_t *packet = (packet_t *)calloc(1, sizeof(packet_t));
  packet->conn = conn;
  packet->length = packetLength;
  packet->startTime = prevTime;
  packet->endTime = prevTime + interArrivalTime;
  return packet;
}

int getRandomNumber(int a, int b) { // range is both inclusive
  return a + rand() % (b - a + 1);
}

void error(const char *message) {
  perror(message);
  exit(1);
}

int main(int argc, char *argv[]) {
  char *inputFile = NULL;
  char *outputFile = NULL;
  srand(time(NULL));

  if (argc != 5)
    error("Invalid input arguments");

  for (int i = 1; i < argc; i++) {
    if (streq(argv[i], "-in", 3))
      inputFile = argv[i + 1];
    else if (streq(argv[i], "-out", 4))
      outputFile = argv[i + 1];
  }

  if (inputFile == NULL || outputFile == NULL)
    error("Invalid input arguments");

  param_t *params = (param_t *)calloc(1, sizeof(param_t));
  conn_list_t *connList = readInputFile(inputFile, params);

  return 0;
}
