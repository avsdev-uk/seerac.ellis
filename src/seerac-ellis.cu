#include "seerac-ellis.hpp"

#include <cstdio>
#include <string>

// Thread block size
#define BLOCKSIZE 1024

#define delta 0.01
#define MCZ 1
#define HAB 2
#define WIND 3
#define COAST 4

#define DATA_PERIOD 1
#define HOLDOUT_PERIOD 2
#define FORECAST_PERIOD 3

//Lookups table ordering
#define PERIODTAB 0
#define REGTAB 1
#define COMPETETAB 2
#define SEASONSPECIESGEARDEPLETION 3 //Ordered to be season on x and species, gear on y
//Held as int, needs to be divided by 65536 to get float.

//Cell Float Data table ordering
#define STARTCOND 0
#define DATAPERIODGEARDEP 1
#define SEASONGEARDEP 2
#define SEASONSPECIESRECOVERY 3
#define HEATMAP 4
#define NUMFLOATTABS 5


__constant__ int d_lookup[MAXLOOKUPWIDTH * MAXLOOKUPHEIGHT];
__constant__ int d_offset[MAXOFFSETS];


__device__ double getSharedFloat(DataMatrix cellData, int cellIdx, int colId)
{
  unsigned long cellRef = cellIdx * cellData.width + colId;
  return cellData.device[cellRef];
}

__device__ void setSharedFloat(ResultsMatrix results, int cellIdx, int periodIdx, int speciesIdx,
  int aryLen, const int *const offset, double* value)
{
  unsigned long cellRef = (cellIdx * results.width) + (periodIdx * offset[NUMSPECIES] * NUMVALUES) +
    (speciesIdx * NUMVALUES);

  for (int idx = 0; idx < aryLen; idx++) {
    results.device[cellRef + idx] = value[idx];
  }
}

__device__ void getLookupData(int tabId, int idx, const int *const offset, int *row)
{
  int rowOffset = 0;

  if (tabId == REGTAB) {
    rowOffset = offset[NUMPERIODS];
  } else if (tabId == COMPETETAB) {
    rowOffset = offset[NUMPERIODS] + offset[NUMREGS];
  } else if (tabId == SEASONSPECIESGEARDEPLETION) {
    rowOffset = offset[NUMPERIODS] + offset[NUMREGS] + offset[NUMCOMPETES];
  }

  unsigned long cellRef = (rowOffset + idx) * offset[LOOKUPWIDTH];
  for (int idx = 0; idx < offset[LOOKUPWIDTH]; idx++) {
    row[idx] = d_lookup[cellRef + idx];
  }
}

__device__ int checkLimit(double dist, double limit)
{
  return (dist <= limit) ? 1 : 0;
}

__device__ int checkBoolean(int val, int mask)
{
  return (val && mask) ? 1 : 0;
}

__device__ int getDataColId(int tab, int colId, const int *const offset)
{
  int seps[NUMFLOATTABS] = {
    2 * offset[NUMSPECIES],
    offset[NUMGEARS] * offset[NUMDATAPERIODS],
    4 * offset[NUMGEARS],
    4 * offset[NUMSPECIES]
  };
  int res = colId;

  for (int idx = 0; idx < tab; idx++) {
    res = res + seps[idx];
  }

  return res;
}

__device__ void calcEffort(double *effortAry, int periodIdx, int cellIdx, const int *const offset,
  DataMatrix cellData)
{
  /* Returns an array of effort for each gear for a given cell at a given period when all
   * applicable restrictions and displacements applied.
   */
  int thisReg[MAXLOOKUPWIDTH];
  int thisPeriod[MAXLOOKUPWIDTH];
  double multiplier = 1.0F;

  getLookupData(PERIODTAB, periodIdx, offset, thisPeriod);

  int periodSeason = thisPeriod[1];

  int bitMask = getSharedFloat(cellData, cellIdx, getDataColId(HEATMAP,0, offset));
  double cellDist = getSharedFloat(cellData, cellIdx, getDataColId(HEATMAP,1, offset));
  int areaCode = getSharedFloat(cellData, cellIdx, getDataColId(HEATMAP,2, offset));
  int cellHab = areaCode & (0x000F);

  for (int idx = 0; idx < offset[NUMGEARS]; idx++) {
    effortAry[idx]=1.0F;
  }

  int areaType, isCoast, isMCZ, isWind, isHab;
  for (int regIdx=0; regIdx < offset[NUMREGS]; regIdx++) {
    multiplier = 1.0F;
    getLookupData(REGTAB, regIdx, offset, thisReg);

    if ((bitMask && (2 ^ (thisReg[1] - 1)) > 0)
      && ((periodSeason == 0) || (periodSeason == thisReg[10]))
      && (periodIdx >= thisReg[2]) && (periodIdx <= thisReg[3])) {

      areaType = thisReg[4];
      isCoast = (areaType == COAST) ? checkLimit(cellDist, thisReg[7]) : 0;
      isMCZ   = (areaType == MCZ)   ? checkBoolean(areaCode, 0x0010)   : 0;
      isWind  = (areaType == WIND)  ? checkBoolean(areaCode, 0x0020)   : 0;
      isHab   = ((areaType == HAB) && (cellHab == thisReg[9])) ? 2 : 0;

      if ((thisReg[2] == 2) && ((isCoast > 0) || (isHab > 0))) {
        multiplier = thisReg[6] / 256.0F; // Used to keep whole table as integers
      } else if ((isCoast > 0) || (isMCZ > 0) || (isWind > 0) || (isHab > 0)) {
        multiplier = 0.0F;
      }

      if ((multiplier != 1.0F) && (thisReg[9] == 0)) {
        for (int idx = 0; idx < offset[NUMGEARS]; idx++) {
          effortAry[idx] = effortAry[idx] * multiplier;
        }
      } else {
        effortAry[thisReg[9]] = effortAry[thisReg[9]] * multiplier;
      }
    }
  }
}

__global__ void testEllis(DataMatrix cellData, ResultsMatrix results)
{
  int cellIdx = blockIdx.x * blockDim.x + threadIdx.x;

  int offset[MAXOFFSETS];
  for (int idx = 0; idx < MAXOFFSETS; idx++) {
    offset[idx] = d_offset[idx];
  }

  int periodRow[MAXLOOKUPWIDTH];     //Used for period data
  int regRow[MAXLOOKUPWIDTH];        //Used for regs data
  int competesRow[MAXLOOKUPWIDTH];   //Used for regs data
  int depletesRow[MAXLOOKUPWIDTH];   //Used for depletion data

  int periodIdx = cellIdx % offset[NUMPERIODS];
  int regIdx = 0;
  if (offset[NUMREGS] > 0) {
    regIdx = cellIdx % offset[NUMREGS];
  }
  int competeIdx = 0;
  if (offset[NUMCOMPETES]) {
    competeIdx = cellIdx % offset[NUMCOMPETES];
  }
  int depleteIdx = cellIdx % (offset[NUMSPECIES] * offset[NUMGEARS]);

  double dummy[] = { 1.01f, 1.05f, 3.05f, 4.5f, 5.8f, 2.3f };
  setSharedFloat(results, cellIdx, 1, 0, NUMVALUES, offset, dummy);

  //Series of tests to prove get, set and lookups are working as expected
  getLookupData(PERIODTAB, periodIdx, offset, periodRow);
  getLookupData(REGTAB, regIdx, offset, regRow);
  getLookupData(COMPETETAB, competeIdx, offset, competesRow);
  getLookupData(SEASONSPECIESGEARDEPLETION, depleteIdx, offset, depletesRow);

  double result[] = {
    (double)periodRow[0],
    (double)regRow[0],
    (double)competesRow[0],
    (double)depletesRow[0],
    cellIdx * (-1.0f),
    100
  };
  setSharedFloat(results, cellIdx, 0, 0, NUMVALUES, offset, result);

  double val0 = getSharedFloat(cellData, cellIdx, 0);
  double val1 = getSharedFloat(cellData, cellIdx, getDataColId(DATAPERIODGEARDEP,0, offset));
  double val2 = getSharedFloat(cellData, cellIdx, getDataColId(SEASONGEARDEP,0, offset));
  double val3 = getSharedFloat(cellData, cellIdx, getDataColId(SEASONSPECIESRECOVERY,0, offset));
  double val4 = getSharedFloat(cellData, cellIdx, getDataColId(HEATMAP, 0, offset));
  double val5 = getSharedFloat(cellData, cellIdx, getDataColId(HEATMAP, 2, offset));

  double result2[] = { val0, val1, val2, val3, val4, val5 };
  setSharedFloat(results, cellIdx, 2, 0, 6, offset, result2);

  double result3[] = { 123.45f, 678.901f };
  setSharedFloat(results, cellIdx, 3, offset[NUMSPECIES] - 1, 2, offset, result3);

  double result4[] = { val5, val4, val3, val2, val1, val0 };
  setSharedFloat(
    results, cellIdx, (offset[NUMPERIODS] - 1), (offset[NUMSPECIES] - 1), NUMVALUES, offset, result4
  );
}

__global__ void calcEllis(DataMatrix cellData, ResultsMatrix results)
{
  int cellIdx = blockIdx.x * blockDim.x + threadIdx.x;

  int offset[MAXOFFSETS];
  for (int idx = 0; idx < MAXOFFSETS; idx++) {
    offset[idx] = d_offset[idx];
  }

  double r = 0.0F,
        dbdt = 0.0F,
        dVal = 1.0F,
        depletion = 0.0F,
        pings = 0.0F,
        sc_n = 0.0F,
        sc_n1 = 0.0F;
  double effortAry[MAXLOOKUPWIDTH];
  int periodRow[MAXLOOKUPWIDTH];   //Used for period data
  int seasonDepRow[MAXLOOKUPWIDTH];  //Used for season depletion data

  for (int periodIdx = 0; periodIdx < offset[NUMPERIODS]; periodIdx++) {
    getLookupData(PERIODTAB, periodIdx, offset, periodRow);
    int season = periodRow[2];

    for (int speciesIdx = 0; speciesIdx < offset[NUMSPECIES]; speciesIdx++) {
      sc_n = (periodIdx == 0) ? getSharedFloat(cellData, cellIdx, speciesIdx) : sc_n1;
      dVal = 1.0F;

      for (int gearIdx = 0; gearIdx < offset[NUMGEARS]; gearIdx++) {
        calcEffort(effortAry, periodIdx, cellIdx, offset, cellData);
        getLookupData(
          SEASONSPECIESGEARDEPLETION, speciesIdx * offset[NUMGEARS] + gearIdx, offset, seasonDepRow
        );
        if (periodRow[1] == 1) {
          pings = getSharedFloat(
            cellData,
            cellIdx,
            getDataColId(DATAPERIODGEARDEP, periodIdx * offset[NUMGEARS] + gearIdx, offset)
          );
        } else {
          pings = getSharedFloat(
            cellData,
            cellIdx,
            getDataColId(SEASONGEARDEP, offset[NUMGEARS] * (season - 1) + gearIdx, offset)
          );
          pings *= effortAry[gearIdx];
        }
        depletion = seasonDepRow[season] / 65536;
        dVal = dVal * pow(depletion, pings);
      } // for gearIdx

      double recRate = getSharedFloat(
        cellData,
        cellIdx,
        getDataColId(SEASONSPECIESRECOVERY, offset[NUMSPECIES] * (season - 1) + speciesIdx, offset)
      );
      double capacity = getSharedFloat(cellData, cellIdx, offset[NUMSPECIES] + speciesIdx);

      //Apply the logistic equation to the starting capacity * depletion and add recovery.....
      double dep = sc_n * dVal;
      dep = dep > 1 ? 1 : dep;

      if (dep > delta)  {
        double x0 = -logf(capacity / dep - 1) / recRate;
        r = capacity / (1 + expf(-recRate * (x0 + 1))) - dep;
        dbdt = sc_n * dVal + r - sc_n;
        sc_n1 = sc_n + dbdt;
      } else if (dep < delta) {
        dVal = 0;
        r = 0;
        dbdt = 0;
        sc_n1 = delta;
      }

      double result[] = { sc_n, dVal, r, dbdt, 0, sc_n1 };
      setSharedFloat(results, cellIdx, periodIdx, speciesIdx, NUMVALUES, offset, result);
    } // for speciesIdx

    for (int competeIdx = 0; competeIdx < offset[NUMCOMPETES]; competeIdx++) {
      /*TO DO - run thru all of the species and assess if they are competing inter species.
      Amortise appropriately use the logistic equation.....
      Probably move the dbdt calc to here as well */
    }
  } // for PeriodIdx
  __syncthreads();
}





#define CU_ERROR_THROWN(cu_error) cuErrorThrown((cu_error), __FILE__, __LINE__)
int cuErrorThrown(cudaError_t cu_errno, const char *const file, int const line)
{
  if (cu_errno != cudaSuccess) {
    fprintf(
      stderr,
      "CUDA ERROR: %s:%d code=%d(%s) \"%s\"\n",
      file, line, (unsigned int)cu_errno, cudaGetErrorName(cu_errno), cudaGetErrorString(cu_errno)
    );
    cudaDeviceReset();
    return 1;
  }
  return 0;
}

int logCudaMemory() {
  double free_m, total_m, used_m;
  size_t free_t, total_t;

  if (CU_ERROR_THROWN(cudaMemGetInfo(&free_t, &total_t))) {
    return 1;
  }

  free_m = free_t / (1024.0 * 1024.0);
  total_m = total_t / (1024.0 * 1024.0);
  used_m = total_m - free_m;

  printf("MEM: mem free %.2f MB; mem total %.2f MB; mem used %.2f MB\n", free_m, total_m, used_m);

  return 0;
}

int hostCalcEllis(int mode, const int *const offset, const int *const lookup, DataMatrix cellData,
  ResultsMatrix results, int debug)
{
  if (debug > 1) {
    printf("Test or real mode: %i\n", mode);
  }

  if (debug) {
    if (logCudaMemory()) {
      return -2;
    }
  }

  if (CU_ERROR_THROWN(cudaMemcpyToSymbol(d_offset, offset, MAXOFFSETS * sizeof(int)))) {
    return -2;
  }

  size_t lookupSize = offset[LOOKUPWIDTH] * offset[LOOKUPHEIGHT] * sizeof(int);
  if (CU_ERROR_THROWN(cudaMemcpyToSymbol(d_lookup, lookup, lookupSize))) {
    return -2;
  }

  size_t inDataSize = cellData.width * cellData.height * sizeof(double);
  if (CU_ERROR_THROWN(cudaMalloc(&cellData.device, inDataSize))) {
    return -2;
  }
  if (CU_ERROR_THROWN(cudaMemcpy(
    cellData.device, cellData.host, inDataSize, cudaMemcpyHostToDevice
    ))) {
    return -2;
  }

  // Allocate results in device memory
  if (debug > 1) {
    printf(
      "Results sized by: numPeriods: %i, numSpecies: %i, numValues: %i\n",
      offset[NUMPERIODS], offset[NUMSPECIES], NUMVALUES
    );
    printf("Mem allocated for results. ncols: %i, nrows: %i\n", results.width, results.height);
  }
  size_t resultsSize = results.width * results.height * sizeof(double);
  if (CU_ERROR_THROWN(cudaMalloc(&results.device, resultsSize))) {
    return -2;
  }


  if (debug) {
    if (logCudaMemory()) {
      return -2;
    }
  }


  if (debug) {
    printf("Running GPU call\n");
  }

  int numBlocks = ceil(cellData.height / BLOCKSIZE);
  if (mode == TEST_MODE) {
    testEllis<<<numBlocks, BLOCKSIZE>>>(cellData, results);
  } else {
    calcEllis<<<numBlocks, BLOCKSIZE>>>(cellData, results);
  }

  if (debug) {
    printf("GPU complete\n");
  }


  if (CU_ERROR_THROWN(cudaMemcpy(
      results.host, results.device, resultsSize, cudaMemcpyDeviceToHost
    ))) {
    return -2;
  }

  cudaFree(cellData.device);
  cudaFree(results.device);

  if (debug > 1) {
    printf("Processing complete\n");
  }

  return 0;
}
