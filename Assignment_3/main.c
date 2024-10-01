#include "main.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

routing_params_t params = {8, 10, 0.5, INQ, 1, 0, "", 0};
static int totalPackets = 0;

router_t *initialiseIO() {
  router_t *router = (router_t *)malloc(sizeof(router_t));
  port_t **inputPorts = (port_t **)malloc(MAX_PORTS * sizeof(port_t *));
  port_t **outputPorts = (port_t **)malloc(MAX_PORTS * sizeof(port_t *));

  for (int i = 0; i < params.ports; i++) {
    port_t *port = (port_t *)malloc(sizeof(port_t));
    port->port = i;
    port->type = INPUT;
    port->bufferCapacity = params.bufferCapacity;
    inputPorts[i] = port;
  }

  for (int i = 0; i < params.ports; i++) {
    port_t *port = (port_t *)malloc(sizeof(port_t));
    port->port = i;
    port->type = OUTPUT;
    port->bufferCapacity = params.knockout;
    outputPorts[i] = port;
  }
  router->numPorts = params.ports;
  router->inputPorts = inputPorts;
  router->outputPorts = outputPorts;
  return router;
}

void generatePackets(router_t *router) {
  port_t **inputPorts = router->inputPorts;
  int numPorts = router->numPorts;
  int outPort;
  int random, threshold = 100 * params.packetGenProb;
  for (int i = 0; i < numPorts; i++) {
    port_t *inputPort = inputPorts[i];
    if (inputPort->bufferCapacity != 0 &&
        inputPort->packetCount == inputPort->bufferCapacity)
      continue;

    random = getRandomNumber(0, 100);
    if (random > threshold)
      continue;

    outPort = getRandomNumber(0, numPorts - 1);
    packet_t *packet = (packet_t *)malloc(sizeof(packet_t));
    packet->id = totalPackets++;
    packet->inPort = i;
    packet->outPort = outPort;
    packet->startTime = router->timeSlot;
    if (inputPort->packetCount++ == 0) {
      inputPort->holPacket = packet;
      inputPort->eolPacket = packet;
    } else {
      inputPort->eolPacket->prevPacket = packet;
      packet->nextPacket = inputPort->eolPacket;
      inputPort->eolPacket = packet;
    }
  }
}

void scheduleNOQ(router_t *router) {
  port_t **inputPorts = router->inputPorts;
  port_t **outputPorts = router->outputPorts;
  for (int i = 0; i < router->numPorts; i++) {
    port_t *outPort = outputPorts[i];
    int *contenders = (int *)malloc(2 * MAX_PORTS * sizeof(int));
    int contenderCount = 0;
    for (int j = 0; j < router->numPorts; j++) {
      port_t *inPort = inputPorts[j];
      if (inPort->holPacket != NULL &&
          inPort->holPacket->outPort == outPort->port) {
        contenders[contenderCount++] = inPort->port;
      }
    }
    if (contenderCount == 0)
      continue;
    int r = getRandomNumber(0, contenderCount - 1);
    int chosenInputPortNumber = contenders[r];
    port_t *chosenInputPort = inputPorts[chosenInputPortNumber];
    packet_t *chosenPacket = chosenInputPort->holPacket;
    sendToOutput(router, chosenPacket);
  }
}

void scheduleCIOQ(router_t *router) {
  packet_t **packets = (packet_t **)malloc(MAX_PORTS * sizeof(packet_t *));
  for (int l = 0; l < params.lookup; l++) {
    for (int i = 0; i < router->numPorts; i++) {
      if (l == 0) {
        port_t *inputPort = router->inputPorts[i];
        packets[i] = inputPort->holPacket;
      }
      if (packets[i] == NULL)
        continue;
      packet_t *packet = packets[i];
      packets[i] = packet->prevPacket;
      sendToOutput(router, packet);
    }
  }
  free(packets);
}

void sendToOutput(router_t *router, packet_t *packet) {
  port_t *outPort = router->outputPorts[packet->outPort];
  port_t *inPort = router->inputPorts[packet->inPort];

  if (outPort->packetCount == outPort->bufferCapacity)
    return;

  if (--inPort->packetCount == 0) {
    inPort->holPacket = NULL;
    inPort->eolPacket = NULL;
  } else if (packet->id == inPort->holPacket->id) {
    inPort->holPacket = inPort->holPacket->prevPacket;
    inPort->holPacket->nextPacket = NULL;
  } else if (packet->id == inPort->eolPacket->id) {
    inPort->eolPacket = packet->nextPacket;
    inPort->eolPacket->prevPacket = NULL;
  } else {
    packet->prevPacket->nextPacket = packet->nextPacket;
    packet->nextPacket->prevPacket = packet->prevPacket;
  }

  if (outPort->packetCount++ == 0) {
    outPort->holPacket = packet;
    outPort->eolPacket = packet;
    return;
  }

  outPort->eolPacket->prevPacket = packet;
  packet->nextPacket = outPort->eolPacket;
  outPort->eolPacket = packet;
  packet->prevPacket = NULL;
  return;
}

void cleanRouterInputs(router_t *router) {
  for (int i = 0; i < router->numPorts; i++) {
    port_t *inputPort = router->inputPorts[i];
    inputPort->holPacket = NULL;
    inputPort->eolPacket = NULL;
    inputPort->packetCount = 0;
  }
}

void schedulePackets(router_t *router) {
  port_t **inputPorts = router->inputPorts;
  int numPorts = router->numPorts;
  for (int i = 0; i < numPorts; i++) {
    port_t *inputPort = inputPorts[i];
    if (inputPort->packetCount == 0)
      continue;
    packet_t *packet = inputPort->holPacket;
    switch (params.scheduleType) {
    case NOQ:
      scheduleNOQ(router);
      break;
    case INQ:
      sendToOutput(router, packet);
      break;
    case CIOQ:
      scheduleCIOQ(router);
      break;
    default:
      error("Scheduling type not supported");
    }
  }
}

int transmitPackets(router_t *router) {
  port_t **outputPorts = router->outputPorts;
  int numPorts = router->numPorts;
  int delay = 0;
  for (int i = 0; i < numPorts; i++) {
    port_t *outPort = outputPorts[i];
    if (outPort->packetCount == 0) {
      outPort->idleSlots++;
      continue;
    }
    outPort->transmittedPackets += outPort->packetCount;
    while (outPort->packetCount > 0) {
      delay += router->timeSlot - outPort->holPacket->startTime;
      if (--outPort->packetCount == 0) {
        outPort->holPacket = NULL;
        outPort->eolPacket = NULL;
      } else {
        outPort->holPacket = outPort->holPacket->prevPacket;
        outPort->holPacket->nextPacket = NULL;
      }
    }
  }
  return delay;
}

void simulate(router_t *router) {
  int totalDelay = 0;
  int numPorts = router->numPorts;
  int maxTimeSlots = params.maxTimeSlots;
  for (int i = 0; i < maxTimeSlots; i++) {
    router->timeSlot = i;
    generatePackets(router);
    schedulePackets(router);
    if (params.bufferCapacity == 0)
      cleanRouterInputs(router);
    totalDelay += transmitPackets(router);
  }
  port_t **outputPorts = router->outputPorts;
  int totalIdleSlots = 0, totalTransmittedPackets = 0;
  for (int i = 0; i < numPorts; i++) {
    totalIdleSlots += outputPorts[i]->idleSlots;
    totalTransmittedPackets += outputPorts[i]->transmittedPackets;
  }
  long double averageUtilisation =
      1 - (long double)totalIdleSlots / (numPorts * maxTimeSlots);
  long double averageDelay = (long double)totalDelay / totalTransmittedPackets;

  printf("average utilization: %Lf\n", averageUtilisation);
  printf("average delay: %Lf\n", averageDelay);
}

int getRandomNumber(int a, int b) { // range is both inclusive
  return a + rand() % (b - a + 1);
}

void error(char *message) {
  perror(message);
  exit(1);
}

int main(int argc, char *argv[]) {
  srand(time(NULL));

  for (int i = 1; i < argc; i++) {
    switch (argv[i][1]) {
    case 'N':
      params.ports = atoi(argv[++i]);
      break;
    case 'B':
      params.bufferCapacity = atoi(argv[++i]);
      break;
    case 'p':
      params.packetGenProb = atof(argv[++i]);
      break;
    case 'q':
      if (streq(argv[++i], "NOQ", 3))
        params.scheduleType = NOQ;
      else if (streq(argv[i], "INQ", 3))
        params.scheduleType = INQ;
      else if (streq(argv[i], "CIOQ", 4))
        params.scheduleType = CIOQ;
      break;
    case 'K':
      params.knockout = atoi(argv[++i]);
      break;
    case 'L':
      params.lookup = atoi(argv[++i]);
      break;
    case 'o':
      strcpy(params.outputfile, argv[++i]);
      break;
    case 'T':
      params.maxTimeSlots = atoi(argv[++i]);
      break;
    }
  }

  switch (params.scheduleType) {
  case NOQ:
    params.bufferCapacity = 0;
    params.lookup = 1;
    params.knockout = 1;
    break;
  case INQ:
    params.lookup = 1;
    params.knockout = 1;
    break;
  case CIOQ:
    if (params.lookup == 0)
      params.lookup = ceil(0.4 * params.ports);
    if (params.knockout == 0)
      params.knockout = ceil(0.4 * params.ports);
    break;
  default:
    error("Scheduling type not supported");
  }

  router_t *router = initialiseIO();
  simulate(router);

  return 0;
}
