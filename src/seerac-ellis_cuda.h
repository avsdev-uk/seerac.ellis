#ifndef __SEERAC_ELLIS_CUDA_H__
#define __SEERAC_ELLIS_CUDA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "host_device_matrix.h"

int getCudaMemory(double *totalMB, double *freeMB, double *usedMB);

int hostCalcEllis(int mode, const int *const offset, const int *const lookup, DataMatrix cellData,
  ResultsMatrix results, int debug);

#ifdef __cplusplus
}
#endif

#endif // __SEERAC_ELLIS_CUDA_HPP__