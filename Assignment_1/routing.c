#include "routing.h"
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define streq(str1, str2, n) (strncmp(str1, str2, n) == 0)

edge_t ***readTopology(const char *filename, int *N, int *E) {
  FILE *file = fopen(filename, "r");
  if (file == NULL)
    error("Error: Unable to open topology file %s\n");

  fscanf(file, "%d %d", N, E);

  edge_t ***edges = (edge_t ***)malloc((*N) * sizeof(edge_t **));
  if (edges == NULL) {
    fclose(file);
    error("Error: Memory allocation failed\n");
  }

  for (int i = 0; i < *N; i++) {
    edges[i] = malloc(*N * sizeof(edge_t *));
    for (int j = 0; j < *N; j++) {
      edges[i][j] = NULL;
    }
  }

  for (int i = 0; i < *E; i++) {
    edge_t *edge = malloc(sizeof(edge_t));
    fscanf(file, "%d %d %d %d", &(edge->node1), &(edge->node2), &(edge->delay),
           &(edge->totalCapacity));
    edge->availableCapacity = edge->totalCapacity;
    edges[edge->node1][edge->node2] = edge;
    edges[edge->node2][edge->node1] = edge;
  }

  fclose(file);
  return edges;
}

request_t **readRequests(const char *filename, int *R) {
  FILE *file = fopen(filename, "r");
  if (file == NULL)
    error("Error: Unable to open file\n");

  fscanf(file, "%d", R);

  request_t **requests = (request_t **)malloc((*R) * sizeof(request_t *));
  if (requests == NULL) {
    fclose(file);
    error("Error: Memory allocation failed\n");
  }

  for (int i = 0; i < *R; i++) {
    requests[i] = (request_t *)malloc(sizeof(request_t));
    fscanf(file, "%d %d %d %d %d", &requests[i]->source,
           &requests[i]->destination, &requests[i]->bMin, &requests[i]->bAvg,
           &requests[i]->bMax);
    requests[i]->pathIndex = -1;
    requests[i]->connected = false;
  }

  fclose(file);
  return requests;
}

node_t **initializeNodes(int N) {
  node_t **nodes = (node_t **)malloc(N * sizeof(node_t *));
  for (int i = 0; i < N; i++) {
    nodes[i] = (node_t *)malloc(sizeof(node_t));
    nodes[i]->node = i;
  }
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

int getPath(int N, int pred[N][N][2], int source, int destination,
            int pathIndex, node_t **path, node_t **nodes) {
  if (destination == source) {
    path[0] = nodes[source];
    return 1;
  }

  if (pred[source][destination][pathIndex] == -1)
    return -1;

  int pathLength = getPath(
      N, pred, source, pred[source][destination][pathIndex], 0, path, nodes);

  if (pathLength == -1)
    return -1;

  path[pathLength] = nodes[destination];
  return pathLength + 1;
}

void compute2ShortestPaths(edge_t ***edges, int N, int E, char *flag,
                           int shortestDist[N][N][2], int pred[N][N][2]) {
  int isHop = streq(flag, "hop", 3);

  int adjMatrix[N][N];
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
      adjMatrix[i][j] = INT_MAX;

  edge_t *edge = NULL;
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      edge = edges[i][j];
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

bool loadRequest(int N, request_t request, node_t **nodes, int pred[N][N][2],
                 int source, int destination, edge_t ***edges, int pathIndex,
                 int p, int finalDestination) {
  if (destination == source)
    return true;

  if (pred[source][destination][pathIndex] == -1)
    return false;

  int prevDestination = pred[source][destination][pathIndex];
  edge_t *edge = edges[prevDestination][destination];
  double bandwidth = bandwidthRequirement(request, p);
  if (edge->availableCapacity < bandwidth)
    return false;

  bool isValid = loadRequest(N, request, nodes, pred, source, prevDestination,
                             edges, 0, p, finalDestination);
  if (!isValid)
    return false;

  node_t *currNode = nodes[prevDestination];
  node_t *nextHop = nodes[destination];
  if (prevDestination == source) {
    currNode->sourceTable[finalDestination] = nextHop->numRequests++;
    nextHop->inputNodes[nextHop->numRequests - 1] = currNode->node;
    currNode->numRequests++;
  } else {
    // currNode->numRequests - 1 is the incoming VCID
    // nextHop->numRequests is the outgoing VCID
    currNode->forwardTable[currNode->numRequests - 1] = nextHop->numRequests++;
    currNode->outputNodes[currNode->numRequests - 1] = nextHop->node;
    nextHop->inputNodes[nextHop->numRequests - 1] = currNode->node;
  }

  edge->availableCapacity -= bandwidth;
  edge->requests[edge->numRequests++] = request;
  return true;
}

double bandwidthRequirement(request_t request, int p) {
  return p == 0 ? fmin(request.bMax,
                       request.bAvg + 0.35 * (request.bMax - request.bMin))
                : request.bMax;
}

void processRequests(int N, node_t **nodes, int pred[N][N][2], edge_t ***edges,
                     request_t **requests, int R, int p) {
  request_t *request;
  for (int i = 0; i < R; i++) {
    request = requests[i];
    bool isShortestPathValid =
        loadRequest(N, *request, nodes, pred, request->source,
                    request->destination, edges, 0, p, request->destination);
    if (!isShortestPathValid) {
      bool isSecondShortestPathValid =
          loadRequest(N, *request, nodes, pred, request->source,
                      request->destination, edges, 1, p, request->destination);
      if (!isSecondShortestPathValid) {
        printf("Connection request failed from %d to %d\n", request->source,
               request->destination);
        request->connected = false;
        continue;
      }
      request->pathIndex = 1;
    } else {
      request->pathIndex = 0;
    }
    request->connected = true;
  }
}

connection_t **createConnections(int N, int R, request_t **requests,
                                 node_t **nodes, int pred[N][N][2],
                                 edge_t ***edges, char *flag) {
  connection_t **connections =
      (connection_t **)malloc(R * sizeof(connection_t *));
  for (int i = 0; i < R; i++) {
    request_t *request = requests[i];
    connections[i] = (connection_t *)malloc(sizeof(connection_t));
    connections[i]->id = i;
    connections[i]->request = request;
    if (!request->connected) {
      connections[i]->connected = false;
      continue;
    }
    node_t **path = (node_t **)malloc(N * sizeof(int));
    for (int j = 0; j < N; j++)
      path[j] = (node_t *)malloc(sizeof(node_t));

    int pathLength = getPath(N, pred, request->source, request->destination,
                             request->pathIndex, path, nodes);
    connection_t *connection = connections[i];
    connection->path = path;
    connection->pathLength = pathLength;
    int pathDelay = 0;
    int pathCost = 0;
    int *vcIDs = (int *)malloc(pathLength * sizeof(int));
    node_t *prevNode = nodes[request->source];
    vcIDs[0] = prevNode->sourceTable[request->destination];
    for (int j = 1; j < pathLength; j++) {
      node_t *node = path[j];
      int delay = edges[prevNode->node][node->node]->delay;
      pathDelay += delay;
      pathCost += streq(flag, "hop", 3) ? 1 : delay;
      prevNode = node;
      vcIDs[j] = node->forwardTable[vcIDs[j - 1]];
    }
    connection->pathDelay = pathDelay;
    connection->pathCost = pathCost;
    connection->vcIDs = vcIDs;
    connection->connected = true;
  }
  return connections;
}

void writeRoutingFile(const char *filename, int N, int pred[N][N][2],
                      node_t **nodes, edge_t ***edges, char *flag) {
  FILE *file = fopen(filename, "w");
  if (file == NULL)
    error("Could not open file");

  fprintf(file, "sourceNode | destinationNode | pathIndex | path | pathDelay | "
                "pathCost\n");

  for (int i = 0; i < N; i++) {
    node_t *sourceNode = nodes[i];
    for (int j = 0; j < N; j++) {
      if (i == j)
        continue;
      node_t *destinationNode = nodes[j];

      node_t **shortestPath = (node_t **)malloc(N * sizeof(int));
      node_t **secondShortestPath = (node_t **)malloc(N * sizeof(int));
      for (int k = 0; k < N; k++)
        shortestPath[k] = (node_t *)malloc(sizeof(node_t));
      for (int k = 0; k < N; k++)
        secondShortestPath[k] = (node_t *)malloc(sizeof(node_t));

      int shortestPathLength =
          getPath(N, pred, sourceNode->node, destinationNode->node, 0,
                  shortestPath, nodes);
      if (shortestPathLength == -1)
        continue;
      int shortestPathDelay = 0;
      int shortestPathCost = 0;
      node_t *prevNode = nodes[sourceNode->node];
      for (int k = 1; k < shortestPathLength; k++) {
        node_t *node = shortestPath[k];
        int delay = edges[prevNode->node][node->node]->delay;
        shortestPathDelay += delay;
        shortestPathCost += streq(flag, "hop", 3) ? 1 : delay;
        prevNode = node;
      }

      fprintf(file, "%d | %d | %d | ", sourceNode->node, destinationNode->node,
              0);
      for (int k = 0; k < shortestPathLength; k++) {
        fprintf(file, "%d", shortestPath[k]->node);
        if (k < shortestPathLength - 1)
          fprintf(file, "->");
      }

      fprintf(file, " | %d | %d\n", shortestPathDelay, shortestPathCost);

      int secondShortestPathLength =
          getPath(N, pred, sourceNode->node, destinationNode->node, 1,
                  secondShortestPath, nodes);
      if (secondShortestPathLength == -1)
        continue;
      int secondShortestPathDelay = 0;
      int secondShortestPathCost = 0;
      prevNode = nodes[sourceNode->node];
      for (int k = 1; k < secondShortestPathLength; k++) {
        node_t *node = secondShortestPath[k];
        int delay = edges[prevNode->node][node->node]->delay;
        secondShortestPathDelay += delay;
        secondShortestPathCost += streq(flag, "hop", 3) ? 1 : delay;
        prevNode = node;
      }

      fprintf(file, "%d | %d | %d | ", sourceNode->node, destinationNode->node,
              1);
      for (int k = 0; k < secondShortestPathLength; k++) {
        fprintf(file, "%d", secondShortestPath[k]->node);
        if (k < secondShortestPathLength - 1)
          fprintf(file, "->");
      }

      fprintf(file, " | %d | %d\n", secondShortestPathDelay,
              secondShortestPathCost);
    }
  }

  fclose(file);
}

void writePathsFile(const char *filename, connection_t **connections,
                    node_t **nodes, int R) {
  FILE *file = fopen(filename, "w");
  if (file == NULL)
    error("Could not open file");

  fprintf(file, "connectionID | sourceNode | destinationNode | Path | VC ID "
                "List | pathCost\n");
  for (int i = 0; i < R; i++) {
    connection_t *connection = connections[i];
    if (!connection->connected)
      continue;

    node_t *sourceNode = nodes[connection->request->source];
    node_t *destinationNode = nodes[connection->request->destination];
    fprintf(file, "%d | %d | %d | ", connection->id, sourceNode->node,
            destinationNode->node);
    node_t **path = connection->path;
    int pathLength = connection->pathLength;
    for (int j = 0; j < pathLength; j++) {
      fprintf(file, "%d", path[j]->node);
      if (j < pathLength - 1)
        fprintf(file, "->");
    }
    fprintf(file, " | ");

    int *vcIDs = connection->vcIDs;
    for (int j = 0; j < pathLength; j++) {
      fprintf(file, "%d", vcIDs[j]);
      if (j < pathLength - 1)
        fprintf(file, ",");
    }
    fprintf(file, " | %d\n", connection->pathCost);
  }
  fclose(file);
}

void writeForwardFile(const char *filename, int N, node_t **nodes) {
  FILE *file = fopen(filename, "w");
  if (file == NULL)
    error("Could not open file");

  fprintf(file, "routerID | incomingNode | VCID | outgoingNode | VCID\n");
  for (int i = 0; i < N; i++) {
    node_t *node = nodes[i];
    for (int j = 0; j < node->numRequests; j++) {
      int incomingVCID = j;
      int outgoingVCID = node->forwardTable[incomingVCID];
      int incomingNode = node->inputNodes[incomingVCID];
      int outgoingNode = node->outputNodes[incomingVCID];
      fprintf(file, "%d | %d | %d | %d | %d\n", node->node, incomingNode,
              incomingVCID, outgoingNode, outgoingVCID);
    }
  }

  fclose(file);
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
  edge_t ***edges = readTopology(topologyFile, &N, &E);

  int shortestDist[N][N][2];
  int pred[N][N][2];

  compute2ShortestPaths(edges, N, E, flag, shortestDist, pred);

  int R;
  node_t **nodes = initializeNodes(N);
  request_t **requests = readRequests(connectionsFile, &R);
  processRequests(N, nodes, pred, edges, requests, R, pValue);
  connection_t **connections =
      createConnections(N, R, requests, nodes, pred, edges, flag);
  writeRoutingFile(routingTableFile, N, pred, nodes, edges, flag);
  writePathsFile(pathsFile, connections, nodes, R);
  writeForwardFile(forwardingFile, N, nodes);

  return 0;
}
