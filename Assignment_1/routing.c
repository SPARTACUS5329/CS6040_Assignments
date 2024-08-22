#include "routing.h"
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define streq(str1, str2, n) (strncmp(str1, str2, n) == 0)

edge_t **readTopology(const char *filename, int *N, int *E) {
  FILE *file = fopen(filename, "r");
  if (file == NULL)
    error("Error: Unable to open topology file %s\n");

  fscanf(file, "%d %d", N, E);

  edge_t **edges = (edge_t **)malloc((*N) * sizeof(edge_t *));
  if (edges == NULL) {
    fclose(file);
    error("Error: Memory allocation failed\n");
  }

  for (int i = 0; i < *N; i++) {
    edges[i] = (edge_t *)malloc(sizeof(edge_t));
    if (edges[i] == NULL) {
      fclose(file);
      error("Error: Memory allocation failed\n");
    }
  }

  int node1, node2, delay, capacity;
  for (int i = 0; i < *E; i++) {
    fscanf(file, "%d %d %d %d", &node1, &node2, &delay, &capacity);
    edges[node1][node2] = (edge_t){node1, node2, delay, capacity, capacity, 0};
    edges[node2][node1] = (edge_t){node2, node1, delay, capacity, capacity, 0};
  }

  fclose(file);
  return edges;
}

request_t *readRequests(const char *filename, int *R) {
  FILE *file = fopen(filename, "r");
  if (file == NULL)
    error("Error: Unable to open file\n");

  fscanf(file, "%d", R);

  request_t *requests = (request_t *)malloc((*R) * sizeof(request_t));
  if (requests == NULL) {
    fclose(file);
    error("Error: Memory allocation failed\n");
  }

  for (int i = 0; i < *R; i++) {
    fscanf(file, "%d %d %d %d %d", &requests[i].source,
           &requests[i].destination, &requests[i].bMin, &requests[i].bAvg,
           &requests[i].bMax);
  }

  fclose(file);
  return requests;
}

node_t *initializeNodes(int N) {
  node_t *nodes = (node_t *)malloc(sizeof(node_t));
  for (int i = 0; i < N; i++)
    nodes[i].node = i;
  return nodes;
}

int compare(const void *a, const void *b) {
  return ((path_node_t *)a)->dist - ((path_node_t *)b)->dist;
}

void initializePaths(int N, int shortestDist[N][N][2], int pred[N][N][2]) {
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      shortestDist[i][j][0] = INT_MAX;
      shortestDist[i][j][1] = INT_MAX;
      pred[i][j][0] = -1;
      pred[i][j][1] = -1;
    }
  }
}

void printPath(int N, int pred[N][N][2], int source, int destination,
               int pathIndex) {
  if (destination == source) {
    printf("%d ", source);
    return;
  }
  if (pred[source][destination][pathIndex] == -1) {
    printf("No path");
    return;
  }
  printPath(N, pred, source, pred[source][destination][pathIndex], 0);
  printf("-> %d ", destination);
}

void compute2ShortestPaths(edge_t **edges, int N, int E, char *flag,
                           int shortestDist[N][N][2], int pred[N][N][2]) {
  int isHop = streq(flag, "hop", 3);

  int adjMatrix[N][N];
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
      adjMatrix[i][j] = INT_MAX;

  edge_t *edge = NULL;
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      edge = &edges[i][j];
      if (edge == NULL)
        continue;
      int u = edge->node1;
      int v = edge->node2;
      int cost = isHop ? 1 : edge->delay;
      adjMatrix[u][v] = cost;
      adjMatrix[v][u] = cost;
    }
  }

  initializePaths(N, shortestDist, pred);

  for (int source = 0; source < N; source++) {
    path_node_t minHeap[N * 2];
    int heapSize = 0;

    minHeap[heapSize++] = (path_node_t){0, source};
    shortestDist[source][source][0] = 0;

    while (heapSize > 0) {
      path_node_t currentNode = minHeap[0];
      heapSize--;
      minHeap[0] = minHeap[heapSize];
      qsort(minHeap, heapSize, sizeof(path_node_t), compare);

      int u = currentNode.node;
      int currentDist = currentNode.dist;

      for (int v = 0; v < N; v++) {
        if (adjMatrix[u][v] == INT_MAX)
          continue;
        int newDist = currentDist + adjMatrix[u][v];

        if (newDist < shortestDist[source][v][0]) {
          shortestDist[source][v][1] = shortestDist[source][v][0];
          pred[source][v][1] = pred[source][v][0];

          shortestDist[source][v][0] = newDist;
          pred[source][v][0] = u;

          minHeap[heapSize++] = (path_node_t){newDist, v};
          qsort(minHeap, heapSize, sizeof(path_node_t), compare);
        } else if (newDist < shortestDist[source][v][1]) {
          shortestDist[source][v][1] = newDist;
          pred[source][v][1] = u;

          minHeap[heapSize++] = (path_node_t){newDist, v};
          qsort(minHeap, heapSize, sizeof(path_node_t), compare);
        }
      }
    }
  }
}

bool loadRequest(int N, request_t request, int pred[N][N][2], int source,
                 int destination, edge_t **edges, int pathIndex, int p) {
  if (destination == source)
    return true;

  if (pred[source][destination][pathIndex] == -1)
    return false;

  int prevDestination = pred[source][destination][pathIndex];
  edge_t edge = edges[prevDestination][destination];
  double bandwidth = bandwidthRequirement(request, p);
  if (edge.availableCapacity < bandwidth)
    return false;

  bool isValid = loadRequest(N, request, pred, source,
                             pred[source][destination][pathIndex], edges, 0, p);
  if (isValid) {
    edge.availableCapacity -= bandwidth;
    edge.requests[edge.numRequests++] = request;
  }
  return isValid;
}

double bandwidthRequirement(request_t request, int p) {
  return p == 0 ? fmin(request.bMax,
                       request.bAvg + 0.35 * (request.bMax - request.bMin))
                : request.bMax;
}

void processRequests(int N, node_t *nodes, int pred[N][N][2], edge_t **edges,
                     request_t *requests, int R, int p) {
  request_t request;
  for (int i = 0; i < R; i++) {
    request = requests[i];
    bool isShortestPathValid = loadRequest(N, request, pred, request.source,
                                           request.destination, edges, 0, p);
    if (isShortestPathValid)
      continue;

    bool isSecondShortestPathValid = loadRequest(
        N, request, pred, request.source, request.destination, edges, 1, p);
    if (!isSecondShortestPathValid)
      printf("Connection request failed from %d to %d: bandwidth requirement "
             "not met\n",
             request.source, request.destination);
  }
}

unsigned long hash(char *str) {
  unsigned long hash = 5381;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return hash % SIZE;
}

forward_table_item_t *searchSymbol(char *key,
                                   forward_table_item_t *hashTable[]) {
  int hashIndex = hash(key);

  while (hashTable[hashIndex] != NULL) {
    if (hashTable[hashIndex]->key == hashIndex)
      return hashTable[hashIndex];

    ++hashIndex;
    hashIndex %= SIZE;
  }

  return NULL;
}

void insertSymbol(char *key, int data, forward_table_item_t *hashTable[]) {
  int hashIndex = hash(key);
  forward_table_item_t *item =
      (forward_table_item_t *)malloc(sizeof(forward_table_item_t));
  item->nextHop = data;
  item->key = hashIndex;

  while (hashTable[hashIndex] != NULL) {
    ++hashIndex;
    hashIndex %= SIZE;
  }

  hashTable[hashIndex] = item;
}

void error(char *message) {
  perror(message);
  exit(1);
}

int main(int argc, char *argv[]) {
  if (argc != 15)
    error("Not enough input arguments");

  char *topologyFile = NULL;
  char *connectionsFile = NULL;
  char *routingTableFile = NULL;
  char *forwardingFile = NULL;
  char *pathsFile = NULL;
  char *flag = NULL;
  int pValue = -1;

  for (int i = 1; i < argc; i++) {
    if (streq(argv[i], "-top", 4))
      topologyFile = argv[++i];
    else if (streq(argv[i], "-conn", 5))
      connectionsFile = argv[++i];
    else if (streq(argv[i], "-rt", 3))
      routingTableFile = argv[++i];
    else if (streq(argv[i], "-ft", 3))
      forwardingFile = argv[++i];
    else if (streq(argv[i], "-path", 5))
      pathsFile = argv[++i];
    else if (streq(argv[i], "-flag", 5)) {
      flag = argv[++i];
      if (!streq(flag, "hop", 3) && !streq(flag, "dist", 4))
        error("Error: Invalid value for -flag. Expected 'hop' or 'dist'.\n");
    } else if (streq(argv[i], "-p", 2) && i + 1 < argc) {
      pValue = atoi(argv[++i]);
      if (pValue != 0 && pValue != 1)
        error("Error: Invalid value for -p. Expected 0 or 1.\n");
    } else
      error("Error: Unknown argument\n");
  }

  int N, E;
  edge_t **edges = readTopology(topologyFile, &N, &E);

  int shortestDist[N][N][2];
  int pred[N][N][2];

  compute2ShortestPaths(edges, N, E, flag, shortestDist, pred);

  int R;
  node_t *nodes = initializeNodes(N);
  request_t *requests = readRequests(connectionsFile, &R);
  processRequests(N, nodes, pred, edges, requests, R, pValue);

  return 0;
}
