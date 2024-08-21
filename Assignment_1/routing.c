#include "routing.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define streq(str1, str2, n) (strncmp(str1, str2, n) == 0)

edge_t *readTopology(const char *filename, int *N, int *E) {
  FILE *file = fopen(filename, "r");
  if (file == NULL)
    error("Error: Unable to open topology file %s\n");

  fscanf(file, "%d %d", N, E);

  edge_t *edges = (edge_t *)malloc((*E) * sizeof(edge_t));
  if (edges == NULL) {
    fclose(file);
    error("Error: Memory allocation failed\n");
  }

  for (int i = 0; i < *E; i++) {
    fscanf(file, "%d %d %d %d", &edges[i].node1, &edges[i].node2,
           &edges[i].delay, &edges[i].capacity);
  }

  fclose(file);
  return edges;
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

void compute2ShortestPaths(edge_t *edges, int N, int E, char *flag) {
  int isHop = streq(flag, "hop", 3);

  int adjMatrix[N][N];
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
      adjMatrix[i][j] = INT_MAX;

  for (int i = 0; i < E; i++) {
    int u = edges[i].node1;
    int v = edges[i].node2;
    int cost = isHop ? 1 : edges[i].delay;
    adjMatrix[u][v] = cost;
    adjMatrix[v][u] = cost;
  }

  int shortestDist[N][N][2];
  int pred[N][N][2];

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

  for (int u = 0; u < N; u++) {
    printf("From node %d:\n", u);
    for (int v = 0; v < N; v++) {
      if (u != v) {
        printf("\tTo node %d:\n", v);
        printf("\t\t1st shortest path: Cost = %d, Path = ",
               shortestDist[u][v][0]);
        printPath(N, pred, u, v, 0);
        printf("\n");

        printf("\t\t2nd shortest path: Cost = %d, Path = ",
               shortestDist[u][v][1]);
        printPath(N, pred, u, v, 1);
        printf("\n");
      }
    }
  }
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
  edge_t *edges = readTopology(topologyFile, &N, &E);
  for (int i = 0; i < E; i++)
    printf("edge_t %d: Node1=%d, Node2=%d, Delay=%d, Capacity=%d\n", i + 1,
           edges[i].node1, edges[i].node2, edges[i].delay, edges[i].capacity);

  compute2ShortestPaths(edges, N, E, flag);

  return 0;
}
