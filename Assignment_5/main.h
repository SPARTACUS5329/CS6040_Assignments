#pragma once
#define MAX_FILE_LENGTH 2000
#define MAX_PREFIX_LENGTH 16
#define MAX_RULES 1000
#define streq(str1, str2, n) (strncmp(str1, str2, n) == 0)

typedef struct Params param_t;
typedef struct Rule rule_t;
typedef struct SubRule sub_rule_t;
typedef struct RuleList rule_list_t;

typedef struct Params {
  char ruleFile[MAX_FILE_LENGTH];
  char inputFile[MAX_FILE_LENGTH];
  char outputFile[MAX_FILE_LENGTH];
} param_t;

typedef struct RuleList {
  int ruleCount;
  rule_t **rules;
} rule_list_t;

typedef struct Rule {
  sub_rule_t *f1;
  sub_rule_t *f2;
} rule_t;

typedef struct SubRule {
  int prefixSize;
  int bits[MAX_PREFIX_LENGTH];
} sub_rule_t;

void error(const char *message);
rule_list_t *readRules(const char *filename);
rule_t *parseRule(const char *line);
