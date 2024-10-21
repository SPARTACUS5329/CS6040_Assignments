#pragma once
#include <pthread.h>
#define streq(str1, str2, n) (strncmp(str1, str2, n) == 0)

typedef struct Param param_t;
typedef struct ConnectionList conn_list_t;
typedef struct Connection conn_t;
typedef struct Packet packet_t;
typedef struct PacketQueue packet_queue_t;

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
  double weight;
  double tStart;
  double tEnd;
  double *interArrivalTimes;
  long double finish;
  pthread_t *thread;
} conn_t;

typedef struct Packet {
  double length;
  double startTime;
  double endTime;
  long double finish;
  conn_t *conn;
} packet_t;

typedef struct PacketQueue {
  int size;
  packet_t **packets;
} packet_queue_t;

void error(const char *message);
int getRandomNumber(int a, int b);
conn_list_t *readInputFile(const char *filename, param_t *params);
long double getInterArrivalTime(double mu);
packet_t *generatePacket(conn_t *conn, double prevTime,
                         double interArrivalTime);
void *connection(void *arg);
void *controller(void *arg);
void *timer(void *arg);
int comparePackets(const void *p1, const void *p2);
long double getFinishNumber(packet_t *packet);
void insertPacket(packet_t *packet);
conn_t *iteratedDeletion();
