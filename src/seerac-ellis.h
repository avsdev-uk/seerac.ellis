#ifndef __SEERAC_ELLIS_H__
#define __SEERAC_ELLIS_H__

#include "host_device_matrix.h"

double getMaxGPUMem();

void setDebug(int debug);
int getDebug();

void setMode(int mode);
int getMode();


int runEllis(const int *const offset, const size_t offsetSize, const int *const lookup,
  const size_t lookupSize, DataMatrix data, ResultsMatrix *results);

int runEllisWithFiles(const int *const offset, const size_t offsetSize);

#endif // __SEERAC_ELLIS_H__