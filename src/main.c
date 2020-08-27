#ifdef WITHOUT_R

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif 
#include <stdio.h>
#include <stdlib.h>

#include "defines.h"
#include "seerac-ellis.h"

int main(int argc, char *argv[])
{
  char *evar;

  evar = secure_getenv("DEBUG");
  if (evar != NULL) {
    setDebug(atoi(evar));
    if (getDebug()) {
      fprintf(stdout, "DEBUG=%s\n", getDebug() ? "ON" : "OFF");
    }
  }

  evar = secure_getenv("MODE");
  if (evar != NULL) {
    setMode(atoi(evar));
    if (getDebug()) {
      fprintf(stdout, "MODE=%s\n", getMode() == 0 ? "NORMAL" : "TEST");
    }
  }

  if ((argc - 1) != MAXOFFSETS) {
    fprintf(stderr, "Not enough offset arguments (%d expected, got %d)\n", MAXOFFSETS, argc - 1);
    return -1;
  }

  int offset[MAXOFFSETS];
  for (int idx = 0; idx < MAXOFFSETS; idx++) {
    offset[idx] = atoi(argv[idx + 1]);
  }

  return runEllisWithFiles(offset, argc - 1);
}

#endif // WITHOUT_R
#ifndef WITHOUT_R
typedef int make_iso_compilers_happy;
#endif