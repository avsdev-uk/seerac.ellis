#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "defines.h"
#include "seerac-ellis.h"
#include "seerac-ellis_cuda.h"

# ifdef WITHOUT_R
#  include <stdio.h>
#  define printT(...) if (config.debug > 1) { fprintf(stdout, __VA_ARGS__); }
#  define printD(...) if (config.debug) { fprintf(stdout, __VA_ARGS__); }
#  define printI(...) fprintf(stdout, __VA_ARGS__)
#  define printW(...) fprintf(stdout, __VA_ARGS__)
#  define printE(...) fprintf(stderr, __VA_ARGS__)
# else
#  include <R_ext/Print.h>
#  include <R_ext/Error.h>
#  define printT(...) if (config.debug > 1) { Rprintf(__VA_ARGS__); } while(0)
#  define printD(...) if (config.debug) { Rprintf(__VA_ARGS__); } while(0)
#  define printI(...) Rprintf(__VA_ARGS__)
#  define printW(...) Rf_warning(__VA_ARGS__)
#  define printE(...) Rf_error(__VA_ARGS__)
# endif // WITHOUT_R


struct t_configuration {
  int debug;
  int mode;
} config = { 0, 0 };


double getMaxGPUMem()
{
  double total;
  if (getCudaMemory(&total, NULL, NULL)) {
    return -1.0;
  }
  return total;
}


void setDebug(int debug) {
  if (debug < 0) {
    printW("Debug < 0. Assuming debug is required to be off (debug = 0)\n");
    debug = 0;
  }
  config.debug = debug;
}
int getDebug() {
  return config.debug;
}

void setMode(int mode) {
  if (mode != TEST_MODE && mode != NORMAL_MODE) {
    printE("mode == %d. I do not know what mode this is meant to be.\n", mode);
    return;
  }
  config.mode = mode;
}
int getMode() {
  return config.mode;
}


int getLookup(const char *const fPath, const int *const offset, int **lookup, size_t *lookupSize)
{
  FILE *fstream;
  char buffer[1024];
  char *record, *line;
  int cellIdx = 0;

  fstream = fopen(fPath,"r");
  if (fstream == NULL) {
    printE("Failed to open lookup file: (%d) %s\n", errno, strerror(errno));
    return -errno;
  }

  (*lookup) = (int *)malloc(offset[LOOKUPHEIGHT] * offset[LOOKUPWIDTH] * sizeof(int));
  if ((*lookup) == 0) {
    printE("Error allocating lookups: (%d) %s\n", errno, strerror(errno));
    return -errno;
  }

  while ((line = fgets(buffer, sizeof(buffer), fstream)) != NULL)
  {
    record = strtok(line, ",");
    while (record != NULL) {
      (*lookup)[cellIdx++] = atoi(record);
      record = strtok(NULL, ",");
    }
  }

  if (lookupSize != 0) {
    *lookupSize = cellIdx;
  }

  return 0;
}

int getDataMatrix(const char *const fPath, DataMatrix *const data)
{
  FILE *fstream;
  char buffer[1024];
  char *record, *line;
  int cellIdx = 0;

  fstream = fopen(fPath,"r");
  if (fstream == NULL) {
    printE("Failed to open data matrix file: (%d) %s\n", errno, strerror(errno));
    return -errno;
  }

  //get the parameters - height, width
  line = fgets(buffer, sizeof(buffer), fstream);
  record = strtok(line, ",");
  data->height = atoi(record);
  record = strtok(NULL, ",");
  data->width = atoi(record);
  //ignore the rest of the line

  data->host = (double *)malloc(data->height * data->width * sizeof(double));
  if (data->host == 0) {
    printE("Error allocating data matrix: (%d) %s\n", errno, strerror(errno));
    return -errno;
  }

  while ((line = fgets(buffer, sizeof(buffer), fstream)) != NULL)
  {
    record = strtok(line,",");
    while (record != NULL) {
      data->host[cellIdx++] = atof(record);
      record = strtok(NULL,",");
    }
  }

  return 0;
}

int saveResults(ResultsMatrix results, const char *fPath)
{
  FILE *outFile;

  outFile = fopen(fPath, "w");
  if (outFile == NULL) {
    printE("Failed to open results file: (%d) %s\n", errno, strerror(errno));
    return -errno;
  }

  // write struct to file
  for (int rowIdx = 0; rowIdx < results.height; rowIdx++) {
    for (int colIdx = 0; colIdx < results.width; colIdx++) {
      fprintf(
        outFile,
        "%.5g%s",
        results.host[rowIdx * results.width + colIdx],
        (colIdx < (results.width - 1) ? "," : "")
      );
    }
    fprintf(outFile,"\n");
  }

  // close file
  fclose(outFile);

  return 0;
}


int runEllis(const int *const offset, const size_t offsetSize, const int *const lookup,
  const size_t lookupSize, DataMatrix data, ResultsMatrix *results)
{
  if (offsetSize < MAXOFFSETS) {
    printE("Offset array is not long enough (got %lu, expected %d)\n", offsetSize, MAXOFFSETS);
    return -1;
  }
  if (offsetSize > MAXOFFSETS) {
    printE("Offset array is too long (got %lu, expected %d)\n", offsetSize, MAXOFFSETS);
    return -1;
  }

  if (lookupSize > MAXLOOKUPWIDTH * MAXLOOKUPHEIGHT) {
    printE(
      "Size of lookup matrix exceeds maximum allowed size (got %lu, max %d)\n",
      lookupSize,
      MAXLOOKUPWIDTH * MAXLOOKUPHEIGHT
    );
    return -1;
  }

  if (results->host == 0) {
    results->height = data.height;
    results->width = offset[NUMPERIODS] * offset[NUMSPECIES] * NUMVALUES;
    results->host = (double *)malloc(results->height * results->width * sizeof(double));
    if (results->host == 0) {
      printE("Error allocating results matrix: (%d) %s\n", errno, strerror(errno));
      return -errno;
    }
  }

  if (config.debug > 1) {
    printI("Offset count: %lu\n", offsetSize);

    printI("Lookup:\n");
    printI("\t first element: %d\n", lookup[0]);
    printI("\t height: %d\n", offset[LOOKUPHEIGHT]);
    printI("\t width: %d\n", offset[LOOKUPWIDTH]);
    printI("\t expected size: %d\n", offset[LOOKUPHEIGHT] * offset[LOOKUPWIDTH]);
    printI("\t actual size: %lu\n", lookupSize);
    printI("\t half way element: %d\n", lookup[lookupSize / 2 + 1]);
    printI("\t last element: %d\n", lookup[(offset[LOOKUPHEIGHT] * offset[LOOKUPWIDTH]) - 10]);

    printI("Data Matrix:\n");
    printI("\t height: %d\n", data.height);
    printI("\t width: %d\n", data.width);
    printI("\t first element: %f\n", data.host[0]);
    printI("\t last element: %f\n", data.host[(data.height * data.width) - 1]);
  }

  printD("Running ellis\n");

  int res = hostCalcEllis(config.mode, offset, lookup, data, *results, config.debug);

  printD("Ellis result: %d\n", res);

  if (config.debug > 1) {
    printI("Results Matrix:\n");
    printI("\theight: %d\n", results->height);
    printI("\twidth: %d\n", results->width);
  }

  return res;
}

int runEllisWithFiles(const int *const offset, const size_t offsetSize) {
  int ret;
  int *lookup;
  DataMatrix data;
  ResultsMatrix results;
  size_t lookupSize = 0;

  printD("Loading data from files\n");

  ret = getLookup("data/simpleLU.csv", offset, &lookup, &lookupSize);
  if (ret != 0) {
    return ret;
  }

  ret = getDataMatrix("data/simpleDM.csv", &data);
  if (ret != 0) {
    free(lookup);
    return ret;
  }

  printD("Data load complete\n");

  //Now we can do some stuff with CUDA!!!
  int res = runEllis(offset, offsetSize, lookup, lookupSize, data, &results);
  if (res != 0) {
    if (results.host != 0) {
      free(results.host);
    }
    free(data.host);
    free(lookup);
    return res;
  }

  res = saveResults(results, "data/results.csv");
  if (res != 0) {
    free(results.host);
    free(data.host);
    free(lookup);
    return res;
  }

  //Clean up time
  free(results.host);
  free(data.host);
  free(lookup);

  return 0;
}