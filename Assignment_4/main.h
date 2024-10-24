#pragma once
#include <pthread.h>
#include <stdbool.h>
#define MAX_CONNECTIONS 20
#define TIME_UNIT 0.1
#define RFB_INTERVAL 10000
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
  bool active;
  long double finish;
  pthread_t thread;
} conn_t;

typedef struct Packet {
  long long int size;
  double length;
  long double genTime;
  long double startTime;
  long double endTime;
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
long double getFinishNumber(packet_t *packet, long double roundNumber);
void insertPacket(packet_queue_t *queue, packet_t *packet);
conn_t *iteratedDeletion(long double roundNumber);
void calculateMetrics();
void heapifyDown(packet_queue_t *queue, int idx);
void heapifyUp(packet_queue_t *queue, int idx);
void insertPacket(packet_queue_t *queue, packet_t *packet);
packet_t *popTopPacket(packet_queue_t *queue);
void calculateMetrics(const char *filename);
