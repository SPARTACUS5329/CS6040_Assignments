#pragma once
#include <stdbool.h>
#define MAX_NODES 200
#define MAX_REQUESTS 200
#define SIZE 2000

typedef struct Request {
  int source;
  int destination;
  int bMin;
  int bAvg;
  int bMax;
  int pathIndex;
} request_t;

typedef struct Edge {
  int node1;
  int node2;
  int delay;
  int totalCapacity;
  double availableCapacity;
  int numRequests;
  request_t requests[MAX_REQUESTS];
} edge_t;

typedef struct Node {
  int node;
  int numRequests;
  int sourceTable[MAX_NODES];
  int forwardTable[MAX_REQUESTS];
} node_t;

typedef struct PathNode {
  int dist;
  int node;
} path_node_t;

void error(char *);
edge_t ***readTopology(const char *, int *, int *);
request_t *readRequests(const char *filename, int *R);
int compare(const void *, const void *);
node_t **initializeNodes(int N);
void initializePaths(int N, int shortestDist[N][N][2], int pred[N][N][2]);
void printPath(int N, int pred[N][N][2], int, int, int);
void compute2ShortestPaths(edge_t ***, int N, int, char *,
                           int shortestDist[N][N][2], int pred[N][N][2]);
double bandwidthRequirement(request_t, int);
bool loadRequest(int N, request_t request, node_t **nodes, int pred[N][N][2],
                 int source, int destination, edge_t ***edges, int pathIndex,
                 int p, int finalDestination);
void processRequests(int N, node_t **nodes, int pred[N][N][2], edge_t ***edges,
                     request_t *requests, int R, int p);
