#include "main.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

param_t *params;

void ipToBinary(const char *ip, int bits[]) {
  int octet1, octet2, octet3, octet4;
  sscanf(ip, "%d.%d.%d.%d", &octet1, &octet2, &octet3, &octet4);

  for (int i = 7; i >= 0; i--) {
    bits[7 - i] = (octet1 >> i) & 1;
    bits[15 - i] = (octet2 >> i) & 1;
  }
}

rule_t *parseRule(const char *line) {
  char ip1[20], ip2[20];
  int prefix1, prefix2;
  rule_t *rule = (rule_t *)calloc(1, sizeof(rule_t));

  if (sscanf(line, "%19[^/]/%d %19[^/]/%d", ip1, &prefix1, ip2, &prefix2) != 4)
    error("Invalid rule format");

  rule->f1 = (sub_rule_t *)calloc(1, sizeof(sub_rule_t));
  rule->f2 = (sub_rule_t *)calloc(1, sizeof(sub_rule_t));

  rule->f1->prefixSize = prefix1;
  rule->f1->rule = rule;
  ipToBinary(ip1, rule->f1->bits);

  rule->f2->prefixSize = prefix2;
  rule->f2->rule = rule;
  ipToBinary(ip2, rule->f2->bits);

  return rule;
}

rule_list_t *readRules(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file)
    error("Could not open rule file");

  rule_list_t *ruleList = (rule_list_t *)calloc(1, sizeof(rule_list_t));
  ruleList->rules = (rule_t **)calloc(MAX_RULES, sizeof(rule_t *));

  char line[100];
  rule_t *rule;

  while (fgets(line, sizeof(line), file)) {
    rule = parseRule(line);
    rule->id = ruleList->ruleCount;
    ruleList->rules[ruleList->ruleCount++] = rule;
  }

  fclose(file);
  return ruleList;
}

void addRule(node_t *root, sub_rule_t *subRule) {
  node_t register *node = root;
  int register bit;
  for (int i = 0; i < subRule->prefixSize; i++) {
    bit = subRule->bits[i];
    switch (bit) {
    case 0:
      if (node->zero == NULL) {
        node->zero = (node_t *)calloc(1, sizeof(node_t));
        node->zero->ruleList = (rule_list_t *)calloc(1, sizeof(rule_list_t));
        node->zero->ruleList->rules =
            (rule_t **)calloc(MAX_RULES, sizeof(rule_t *));
      }
      node = node->zero;
      break;
    case 1:
      if (node->one == NULL) {
        node->one = (node_t *)calloc(1, sizeof(node_t));
        node->one->ruleList = (rule_list_t *)calloc(1, sizeof(rule_list_t));
        node->one->ruleList->rules =
            (rule_t **)calloc(MAX_RULES, sizeof(rule_t *));
      }
      node = node->one;
      break;
    default:
      error("Invalid bit encountered");
    }
  }

  node->isPrefix = true;
  node->ruleList->rules[node->ruleList->ruleCount++] = subRule->rule;
}

node_t *createSearchTrie(rule_list_t *ruleList, rule_field_e ruleField) {
  rule_t *rule;
  node_t *root = (node_t *)calloc(1, sizeof(node_t));
  root->ruleList = (rule_list_t *)calloc(1, sizeof(rule_list_t));
  root->ruleList->rules = (rule_t **)calloc(MAX_RULES, sizeof(rule_t *));

  for (int i = 0; i < ruleList->ruleCount; i++) {
    rule = ruleList->rules[i];
    switch (ruleField) {
    case ONE:
      addRule(root, rule->f1);
      break;
    case TWO:
      addRule(root, rule->f2);
      break;
    default:
      error("Invalid rule field type");
    }
  }

  return root;
}

void createF2Trie(node_t *node, rule_list_t *ruleList) {
  if (node == NULL)
    return;

  node->f2Root = createSearchTrie(node->ruleList, TWO);

  createF2Trie(node->zero, ruleList);
  createF2Trie(node->one, ruleList);
}

node_t *createHierarchicalTrie(rule_list_t *ruleList) {
  node_t *f1Root = createSearchTrie(ruleList, ONE);
  createF2Trie(f1Root, ruleList);
  return f1Root;
}

ip_t *parseIp(const char *ipStr1, const char *ipStr2) {
  ip_t *ip = (ip_t *)calloc(1, sizeof(ip_t));
  ip->ruleList = (rule_list_t *)calloc(1, sizeof(rule_list_t));
  ip->ruleList->rules = (rule_t **)calloc(MAX_RULES, sizeof(rule_t *));
  ip->length = MAX_PREFIX_LENGTH;

  strcpy(ip->f1, ipStr1);
  strcpy(ip->f2, ipStr2);

  ipToBinary(ipStr1, ip->f1Bits);
  ipToBinary(ipStr2, ip->f2Bits);
  return ip;
}

ip_list_t *readIpList(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file)
    error("Could not open input file");

  ip_list_t *ipList = (ip_list_t *)calloc(1, sizeof(ip_list_t));
  ipList->ips = (ip_t **)calloc(MAX_IP_COUNT, sizeof(ip_t *));

  char line[100];
  while (fgets(line, sizeof(line), file)) {
    char ipStr1[20], ipStr2[20];

    if (sscanf(line, "%19s %19s", ipStr1, ipStr2) != 2)
      error("Invalid IP entry");

    ipList->ips[ipList->ipCount++] = parseIp(ipStr1, ipStr2);
  }

  fclose(file);
  return ipList;
}

void getApplicableRules(node_t *f2Root, ip_t *ip) {
  int bit;
  node_t *node = f2Root;
  for (int i = 0; i < ip->length; i++) {
    if (node == NULL)
      break;

    if (node->isPrefix) {
      for (int j = 0; j < node->ruleList->ruleCount; j++)
        ip->ruleList->rules[ip->ruleList->ruleCount++] =
            node->ruleList->rules[j];
    }

    bit = ip->f2Bits[i];
    switch (bit) {
    case 0:
      node = node->zero;
      break;
    case 1:
      node = node->one;
      break;
    default:
      error("Invalid bit");
    }
  }
}

void resolveIP(node_t *f1Root, ip_t *ip) {
  clock_t start, end;

  start = clock();

  int bit;
  node_t *node = f1Root;
  for (int i = 0; i < ip->length; i++) {
    if (node == NULL)
      break;

    if (node->isPrefix)
      getApplicableRules(node->f2Root, ip);

    bit = ip->f1Bits[i];
    switch (bit) {
    case 0:
      node = node->zero;
      break;
    case 1:
      node = node->one;
      break;
    default:
      error("Invalid bit");
    }
  }

  end = clock();

  ip->searchTime = ((double)(end - start)) / CLOCKS_PER_SEC;
}

void writeOutput(const char *filename, ip_list_t *ipList) {
  int saved_stdout = dup(fileno(stdout));
  FILE *file = freopen(filename, "w", stdout);
  if (file == NULL)
    error("Error opening file");

  ip_t *ip;
  rule_t *rule;
  double totalSearchTime = 0;

  for (int i = 0; i < ipList->ipCount; i++) {
    ip = ipList->ips[i];
    printf("%s", ip->f1);
    printf("\t");
    printf("%s", ip->f2);
    printf("\t");
    printf("%d", ip->ruleList->ruleCount);
    printf("\t");
    for (int j = 0; j < ip->ruleList->ruleCount; j++) {
      rule = ip->ruleList->rules[j];
      printf("%d", rule->id);
      printf(" ");
    }
    printf("%lf", ip->searchTime * 1e6);
    printf("\n");
    totalSearchTime += ip->searchTime * 1e6;
  }

  fflush(stdout);
  dup2(saved_stdout, fileno(stdout));
  close(saved_stdout);

  printf("Average Search Time = %lf µs\n", totalSearchTime / ipList->ipCount);
}

void error(const char *message) {
  perror(message);
  exit(1);
}

int main(int argc, char *argv[]) {
  params = (param_t *)calloc(1, sizeof(param_t));

  for (int i = 1; i < argc; i++)
    if (streq(argv[i], "-p", 2) && ++i < argc)
      sprintf(params->ruleFile, "%s", argv[i]);
    else if (streq(argv[i], "-i", 2) && ++i < argc)
      sprintf(params->inputFile, "%s", argv[i]);
    else if (streq(argv[i], "-o", 2) && ++i < argc)
      sprintf(params->outputFile, "%s", argv[i]);

  rule_list_t *ruleList = readRules(params->ruleFile);
  ip_list_t *ipList = readIpList(params->inputFile);

  node_t *f1Root = createHierarchicalTrie(ruleList);

  ip_t *ip;
  rule_t *rule;
  for (int i = 0; i < ipList->ipCount; i++) {
    ip = ipList->ips[i];
    resolveIP(f1Root, ip);
  }

  writeOutput(params->outputFile, ipList);

  return 0;
}
