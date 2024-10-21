#include "main.h"
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static clock_t simulationStartTime;
static conn_list_t *connList;
param_t *params;

packet_queue_t *packetQueue;
long double roundNumber;
static long double clockTime;

pthread_mutex_t packetQueueMutex;
pthread_mutex_t roundNumberMutex;

conn_list_t *readInputFile(const char *filename, param_t *params) {
  FILE *file = fopen(filename, "r");
  if (file == NULL)
    error("Failed to open input file");

  int totalConnections = 0;

  fscanf(file, "N=%d T=%d C=%d B=%d", &params->N, &params->T, &params->C,
         &params->B);

  packetQueue->packets = (packet_t **)malloc(params->B * sizeof(packet_t *));

  conn_list_t *connList = (conn_list_t *)malloc(sizeof(conn_list_t));
  connList->connCount = params->N;
  connList->conns = (conn_t **)malloc(params->N * sizeof(conn_t *));

  for (int i = 0; i < params->N; i++) {
    conn_t *conn = (conn_t *)calloc(1, sizeof(conn_t));
    conn->id = totalConnections++;
    conn->thread = (pthread_t *)malloc(sizeof(pthread_t));

    fscanf(file, "%d %d %d %lf %lf %lf", &conn->p, &conn->lMin, &conn->lMax,
           &conn->weight, &conn->tStart, &conn->tEnd);

    connList->conns[i] = conn;
  }

  fclose(file);
  return connList;
}

long double getInterArrivalTime(double mu) {
  double uniformRandom = (double)rand() / RAND_MAX;
  return (long double)-log(1 - uniformRandom) / mu;
}

packet_t *generatePacket(conn_t *conn, double prevTime,
                         double interArrivalTime) {
  int packetLength = getRandomNumber(conn->lMin, conn->lMax);
  packet_t *packet = (packet_t *)calloc(1, sizeof(packet_t));
  packet->conn = conn;
  packet->length = (double)packetLength / params->C;
  return packet;
}

void *connection(void *arg) {
  conn_t *conn = (conn_t *)arg;
  long double time;
  long double startTime = conn->tStart * params->T;
  long double endTime = conn->tEnd * params->T;

  printf("%d endTime: %Lf\n", conn->id, endTime);
  long double interArrivalTime;
  double mu = (double)1 / conn->p;

  while (true) {
    time = clockTime;
    if (time < startTime)
      continue;
    if (time > endTime)
      break;
    interArrivalTime = getInterArrivalTime(mu);
    packet_t *packet = generatePacket(conn, time, interArrivalTime);
    insertPacket(packet);
    usleep(interArrivalTime * 100); // conversion to micro seconds
  }

  printf("Exiting: %d %Lf\n", conn->id, time);

  pthread_exit(NULL);
  return NULL;
}

void *controller(void *arg) {
  int bufferSize = 0;
  long double slopeFactor, prevEventTime, time;
  packet_t *transmittingPacket = NULL;
  conn_t *conn;

  while (true) {
    time = clockTime;
    if (time > params->T)
      break;

    pthread_mutex_lock(&packetQueueMutex);

    if (packetQueue->size > bufferSize || packetQueue->size == params->B) {
      roundNumber += (time - prevEventTime) * slopeFactor;
      prevEventTime = time;

      for (int i = 0; i < packetQueue->size; i++) {
        packet_t *packet = packetQueue->packets[i];
        // new packet check
        // can't start from i = bufferSize since queue might have reordered
        if (packet->finish == 0) {
          packet->finish = getFinishNumber(packet);
          packet->conn->finish = packet->finish;
          slopeFactor += packet->conn->weight;
        }
      }

      qsort(packetQueue->packets, packetQueue->size, sizeof(packet_t *),
            comparePackets);
      bufferSize = packetQueue->size;
    }

    pthread_mutex_unlock(&packetQueueMutex);

    if (transmittingPacket == NULL) {
      pthread_mutex_lock(&packetQueueMutex);

      if (packetQueue->size == 0)
        goto release_queue;

      transmittingPacket = packetQueue->packets[0];
      // corner case: queue was empty initially but packet was added in between
      if (transmittingPacket->finish == 0)
        goto release_queue;

      transmittingPacket->startTime = time;
      transmittingPacket->endTime =
          transmittingPacket->startTime + transmittingPacket->length;

      packetQueue->packets[0] = packetQueue->packets[--packetQueue->size];
      qsort(packetQueue->packets, packetQueue->size, sizeof(packet_t *),
            comparePackets);
      bufferSize = packetQueue->size;

    release_queue:
      pthread_mutex_unlock(&packetQueueMutex);
      continue;
    }

    if (time < transmittingPacket->endTime)
      continue;

    roundNumber += (time - prevEventTime) / slopeFactor;
    while ((conn = iteratedDeletion()) != NULL) {
      long double inactiveTime = prevEventTime + conn->finish * slopeFactor;
      slopeFactor -= conn->weight;
      prevEventTime = inactiveTime;
      roundNumber += (time - prevEventTime) / slopeFactor;
    }

    prevEventTime = time;
    transmittingPacket = NULL;
  }

  printf("Controller thread exit\n");

  pthread_exit(NULL);
  return NULL;
}

conn_t *iteratedDeletion() {
  for (int i = 0; i < connList->connCount; i++)
    if (connList->conns[i]->finish < roundNumber)
      return connList->conns[i];

  return NULL;
}

long double getFinishNumber(packet_t *packet) {
  return fmax(packet->conn->finish, roundNumber) +
         (long double)packet->length / packet->conn->weight;
}

void insertPacket(packet_t *packet) {
  pthread_mutex_lock(&packetQueueMutex);
  if (packetQueue->size == params->B) {
    pthread_mutex_unlock(&packetQueueMutex);
    return;
  }

  packetQueue->packets[packetQueue->size++] = packet;
  pthread_mutex_unlock(&packetQueueMutex);
}

int comparePackets(const void *p1, const void *p2) {
  return ((packet_t *)p1)->finish - ((packet_t *)p2)->finish;
}

void *timer(void *arg) {
  while (true) {
    clockTime =
        1e4 * ((long double)(clock() - simulationStartTime) / CLOCKS_PER_SEC);
    if (clockTime > params->T)
      break;
  }
  pthread_exit(NULL);
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

  params = (param_t *)calloc(1, sizeof(param_t));
  packetQueue = (packet_queue_t *)malloc(sizeof(packet_queue_t));
  connList = readInputFile(inputFile, params);

  simulationStartTime = clock();

  pthread_t *timerThread = (pthread_t *)malloc(sizeof(pthread_t));
  if (pthread_create(timerThread, NULL, timer, NULL) != 0)
    error("[Thread creation] Timer thread not created\n");

  pthread_t *controllerThread = (pthread_t *)malloc(sizeof(pthread_t));
  if (pthread_create(controllerThread, NULL, controller, NULL) != 0)
    error("[Thread creation] Controller thread not created\n");

  conn_t *conn;
  for (int i = 0; i < connList->connCount; i++) {
    conn = connList->conns[i];
    if (pthread_create(conn->thread, NULL, connection, conn) != 0)
      error("[Thread creation] Connection thread not created \n");
  }

  for (int i = 0; i < connList->connCount; i++) {
    conn = connList->conns[i];
    pthread_join(*conn->thread, NULL);
  }

  pthread_join(*controllerThread, NULL);
  pthread_join(*timerThread, NULL);

  return 0;
}
