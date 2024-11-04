// This program generates input rules for the class. table based on 2 fields
// Generate executable file called say 'run'
// ./run  output_file seed num_rules
// seed is an integer
// Different seed values generate different output files.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

void genfield2(FILE *fp) {
  long lrand, nrand, frand;
  long tempfield, i, j;

  lrand = random() % 100;

  if (lrand <= 33)
    fprintf(fp, "%ld.0.0.0/%d ", random() % 255, 8);
  else {
    if (lrand <= 67) {
      nrand = (random() % 8 + 1); /* nrand: 1..8 */
      tempfield = 0;
      for (i = 0; i < nrand; i++) {
        j = random() % 2;
        tempfield += j * pow(2, 7 - i);
      }
      //	  fprintf(fp,"\n%ld: %ld\n", nrand, (long) pow(2,8-nrand));
      fprintf(fp, "%ld.%ld.0.0/%ld ", random() % 255, tempfield, 8 + nrand);
    } else {
      nrand = (random() % 8 + 1); /* nrand: 1..8 */
      tempfield = 0;
      for (i = 0; i < nrand; i++) {
        j = random() % 2;
        tempfield += j * pow(2, 7 - i);
      }
      //	  fprintf(fp,"\n%ld: %ld\n", nrand, (long) pow(2,8-nrand));
      fprintf(fp, "%ld.%ld.%ld.0/%ld ", random() % 255, random() % 255,
              tempfield, 16 + nrand);
    }
  }
}

int main(int argc, char **argv) {
  int i, j, k;
  FILE *fp;
  int num_rules;

  if (argc < 3) {
    fprintf(stdout, "Usage: %s opfile  num_rules seed num_rules \n", argv[0]);
    exit(-1);
  }

  fp = fopen(argv[1], "w");
  srandom(atoi(argv[2]));
  num_rules = atoi(argv[3]);

  for (i = 0; i < num_rules; i++) {
    genfield2(fp);
    genfield2(fp);
    fprintf(fp, "\n");
  }
}
