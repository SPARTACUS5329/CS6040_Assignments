#pragma once
#include <stdbool.h>
#define MAX_FILE_LENGTH 2000
#define MAX_PREFIX_LENGTH 32
#define MAX_IP_COUNT 1000000
#define MAX_IP_LENGTH 32
#define MAX_RULES 300000
#define MAX_IP_STING_LENGTH 200
#define streq(str1, str2, n) (strncmp(str1, str2, n) == 0)

typedef struct Params param_t;
typedef struct RuleList rule_list_t;
typedef struct Rule rule_t;
typedef struct SubRule sub_rule_t;
typedef struct RuleList rule_list_t;
typedef struct Node node_t;
typedef struct IpList ip_list_t;
typedef struct Ip ip_t;

typedef enum { ONE, TWO } rule_field_e;

typedef struct Params {
  char ruleFile[MAX_FILE_LENGTH];
  char inputFile[MAX_FILE_LENGTH];
  char outputFile[MAX_FILE_LENGTH];
} param_t;

typedef struct IpList {
  int ipCount;
  ip_t **ips;
} ip_list_t;

typedef struct Ip {
  int length;
  double searchTime;
  char f1[MAX_IP_STING_LENGTH];
  char f2[MAX_IP_STING_LENGTH];
  int f1Bits[MAX_IP_LENGTH];
  int f2Bits[MAX_IP_LENGTH];
  rule_list_t *ruleList;
} ip_t;

typedef struct RuleList {
  int ruleCount;
  rule_t **rules;
} rule_list_t;

typedef struct Rule {
  int id;
  sub_rule_t *f1;
  sub_rule_t *f2;
} rule_t;

typedef struct SubRule {
  int prefixSize;
  int bits[MAX_PREFIX_LENGTH];
  rule_t *rule;
} sub_rule_t;

typedef struct Node {
  bool isPrefix;
  rule_list_t *ruleList;
  node_t *zero;
  node_t *one;
  node_t *f2Root;
} node_t;

void error(const char *message);
rule_list_t *readRules(const char *filename);
rule_t *parseRule(const char *line);
void ipToBinary(const char *ip, int bits[]);
node_t *createHierarchicalTrie(rule_list_t *ruleList);
node_t *createSearchTrie(rule_list_t *ruleList, rule_field_e ruleField);
void addRule(node_t *root, sub_rule_t *subRule);
ip_t *parseIp(const char *ipStr1, const char *ipStr2);
ip_list_t *readIpList(const char *filename);
void resolveIP(node_t *f1Root, ip_t *ip);
void getApplicableRules(node_t *f2Root, ip_t *ip);
void writeOutput(const char *filename, ip_list_t *ipList);
