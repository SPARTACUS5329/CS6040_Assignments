#pragma once
#define MAX_PACKETS 2000
#define MAX_PORTS 2000
#define streq(str1, str2, n) (strncmp(str1, str2, n) == 0)

typedef struct RoutingParams routing_params_t;
typedef struct Router router_t;
typedef struct Packet packet_t;
typedef struct Port port_t;

typedef enum { NOQ, INQ, CIOQ } schedule_type_e;
typedef enum { INPUT, OUTPUT } port_e;

typedef struct RoutingParams {
  int ports;
  int bufferCapacity;
  float packetGenProb;
  schedule_type_e scheduleType;
  int knockout;
  int lookup;
  char outputfile[100];
  int maxTimeSlots;
} routing_params_t;

typedef struct Router {
  int numPorts;
  int timeSlot;
  port_t **inputPorts;
  port_t **outputPorts;
} router_t;

typedef struct Packet {
  int inPort;
  int outPort;
  int startTime;
  packet_t *prevPacket;
  packet_t *nextPacket;
} packet_t;

typedef struct Port {
  port_e type;
  int port;
  int bufferCapacity;
  int packetCount;
  packet_t *holPacket;
  packet_t *eolPacket;
} port_t;

void error(char *message);
int getRandomNumber(int a, int b);
router_t *initialiseIO();
void simulate(router_t *router);
void generatePackets(router_t *router);
void schedulePackets(router_t *router);
void sendToOutput(router_t *router, packet_t *packet);
int transmitPackets(router_t *router);
void cleanRouterInputs(router_t *router);
void scheduleNOQ(router_t *router);
