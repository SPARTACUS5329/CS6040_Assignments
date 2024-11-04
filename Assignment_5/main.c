#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  ipToBinary(ip1, rule->f1->bits);

  rule->f2->prefixSize = prefix2;
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

  while (fgets(line, sizeof(line), file) && ruleList->ruleCount < MAX_RULES)
    ruleList->rules[ruleList->ruleCount++] = parseRule(line);

  fclose(file);
  return ruleList;
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

  return 0;
}
