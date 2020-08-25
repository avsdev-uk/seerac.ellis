#ifndef __SEERAC_ELLIS_CUDA_HPP__
#define __SEERAC_ELLIS_CUDA_HPP__

#define NUMPERIODS 0
#define NUMDATAPERIODS 1
#define NUMREGS 2
#define NUMCOMPETES 3
#define NUMGEARS 4
#define NUMSPECIES 5
#define LOOKUPHEIGHT 6
#define LOOKUPWIDTH 7
#define MAXOFFSETS 8

#define MAXLOOKUPWIDTH 16
#define MAXLOOKUPHEIGHT 512

// Why is this 6?
#define NUMVALUES 6

#define NORMAL_MODE 0
#define TEST_MODE 1

struct HostDeviceMatrix {
  int width = 0;
  int height = 0;
  double *host = 0;
  double *device = 0;
};
typedef HostDeviceMatrix DataMatrix;
typedef HostDeviceMatrix ResultsMatrix;

int hostCalcEllis(int mode, const int *const offset, const int *const lookup, DataMatrix cellData,
  ResultsMatrix results, int debug = 0);

#endif // __SEERAC_ELLIS_CUDA_HPP__