#pragma once

typedef struct Edge {
  int node1;
  int node2;
  int delay;
  int capacity;
} edge_t;

typedef struct PathNode {
  int dist;
  int node;
} path_node_t;

void error(char *);
edge_t *readTopology(const char *, int *, int *);
int compare(const void *, const void *);
void initializePath(int N, int shortestDist[N][N][2], int pred[N][N][2]);
void printPath(int N, int pred[N][N][2], int, int, int);
void compute2ShortestPaths(edge_t *, int, int, char *);
