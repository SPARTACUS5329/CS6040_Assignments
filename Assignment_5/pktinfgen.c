// Used to generate testing file as input for checking your program's
// classification. Compile to generate executable as run The input file contains
// the set of rules stored in the packet class. table.
// ./run inpfile outputfile

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

void gen_match(int *addr1, int *addr2, int len1, int len2, FILE *fpout) {
  /*       fprintf(stdout, "%d.%d.%d.%d/%d %d.%d.%d.%d/%d\n",  */
  /* 	      addr1[0], addr1[1],  addr1[2], addr1[3], len1, */
  /* 	      addr2[0], addr2[1],  addr2[2], addr2[3], len2 */
  /* 	      ); */

  /* First Field */
  if (len1 <= 8)
    fprintf(fpout, "%d.%ld.%ld.%ld ", addr1[0], random() % 255, random() % 255,
            random() % 255);
  else if (len1 <= 16) {
    fprintf(fpout, "%d.%ld.%ld.%ld ", addr1[0],
            addr1[1] + random() % ((long)pow(2, 16 - len1)), random() % 255,
            random() % 255);
  } else {
    fprintf(fpout, "%d.%d.%ld.%ld ", addr1[0], addr1[1],
            addr1[2] + random() % ((long)pow(2, len1 - 16)), random() % 255);
  }

  /* Second Field */
  if (len2 <= 8)
    fprintf(fpout, "%d.%ld.%ld.%ld\n", addr2[0], random() % 255, random() % 255,
            random() % 255);
  else if (len2 <= 16) {
    fprintf(fpout, "%d.%ld.%ld.%ld\n", addr2[0],
            addr2[1] + random() % ((long)pow(2, 16 - len2)), random() % 255,
            random() % 255);
  } else {
    fprintf(fpout, "%d.%d.%ld.%ld\n", addr2[0], addr2[1],
            addr2[2] + random() % ((long)pow(2, len2 - 16)), random() % 255);
  }
}

void gen_nomatch(int *addr1, int *addr2, int len1, int len2, FILE *fpout) {
  /* Field 1 is same as original or random values */
  if (random() % 1 == 0)
    fprintf(fpout, "%ld.%ld.%ld.%ld ", random() % 255, random() % 255,
            random() % 255, random() % 255);
  else
    fprintf(fpout, "%d.%d.%d.%d ", addr1[0], addr1[1], addr1[2], addr1[3]);

  /* Random field 2 */
  fprintf(fpout, "%ld.%ld.%ld.%ld\n", random() % 255, random() % 255,
          random() % 255, random() % 255);
}

int main(int argc, char **argv) {
  int i, j, k;
  FILE *fpin, *fpout;
  int addr1[4], len1, addr2[4], len2;

  fpin = fopen(argv[1], "r");
  fpout = fopen(argv[2], "w");

  j = fscanf(fpin, "%d.%d.%d.%d/%d %d.%d.%d.%d/%d", &addr1[0], &addr1[1],
             &addr1[2], &addr1[3], &len1, &addr2[0], &addr2[1], &addr2[2],
             &addr2[3], &len2);

  while (j != EOF) {
    gen_match(addr1, addr2, len1, len2, fpout);
    gen_nomatch(addr1, addr2, len1, len2, fpout);
    gen_match(addr1, addr2, len1, len2, fpout);

    j = fscanf(fpin, "%d.%d.%d.%d/%d %d.%d.%d.%d/%d", &addr1[0], &addr1[1],
               &addr1[2], &addr1[3], &len1, &addr2[0], &addr2[1], &addr2[2],
               &addr2[3], &len2);
  }
}
