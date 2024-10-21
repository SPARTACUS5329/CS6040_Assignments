#pragma once
#include <pthread.h>
#define streq(str1, str2, n) (strncmp(str1, str2, n) == 0)

typedef struct Param param_t;
typedef struct ConnectionList conn_list_t;
typedef struct Connection conn_t;
typedef struct Packet packet_t;

typedef struct Param {
  int N;
  int T;
  int C;
  int B;
} param_t;

typedef struct ConnectionList {
  int connCount;
  conn_t **conns;
} conn_list_t;

typedef struct Connection {
  int id;
  int p;
  int lMin;
  int lMax;
  int w;
  double tStart;
  double tEnd;
  double *interArrivalTimes;
  pthread_t *thread;
} conn_t;

typedef struct Packet {
  conn_t *conn;
  double startTime;
  double endTime;
  int length;
} packet_t;

void error(const char *message);
int getRandomNumber(int a, int b);
conn_list_t *readInputFile(const char *filename, param_t *params);
double generateInterArrivalTimes(double mu);
packet_t *generatePacket(conn_t *conn, double prevTime,
                         double interArrivalTime);
void *connection(void *arg);
void *controller(void *arg);
