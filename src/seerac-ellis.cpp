// System includes
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#ifdef WITHOUT_R
# define Rprintf(...) fprintf(stdout, __VA_ARGS__)
# define REprintf(...) fprintf(stderr, __VA_ARGS__)
#else
# include <R.h>
# include <Rdefines.h>
# include <Rinternals.h>
#endif

#include "seerac-ellis.hpp"


struct t_configuration {
  int debug = 1; // 0;
  int mode = TEST_MODE; //NORMAL_MODE;
} config;


int getLookup(const char *const fPath, const int *const offset, int **lookup, size_t *lookupSize)
{
  FILE *fstream;
  char buffer[1024];
  char *record, *line;
  int cellIdx = 0;

  fstream = fopen(fPath,"r");
  if (fstream == NULL) {
    REprintf("Failed to open lookup file: (%d) %s\n", errno, strerror(errno));
    return -errno;
  }

  (*lookup) = (int *)malloc(offset[LOOKUPHEIGHT] * offset[LOOKUPWIDTH] * sizeof(int));
  if ((*lookup) == 0) {
    REprintf("Error allocating lookups: (%d) %s\n", errno, strerror(errno));
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
    REprintf("Failed to open data matrix file: (%d) %s\n", errno, strerror(errno));
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
    REprintf("Error allocating data matrix: (%d) %s\n", errno, strerror(errno));
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
    REprintf("Failed to open results file: (%d) %s\n", errno, strerror(errno));
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
  if (results->host == 0) {
    results->height = data.height;
    results->width = offset[NUMPERIODS] * offset[NUMSPECIES] * NUMVALUES;
    results->host = (double *)malloc(results->height * results->width * sizeof(double));
    if (results->host == 0) {
      REprintf("Error allocating results matrix: (%d) %s\n", errno, strerror(errno));
      return -errno;
    }
  }

  if (config.debug > 1) {
    Rprintf("Offset count: %i\n", offsetSize);

    Rprintf("Lookup:\n");
    Rprintf("\t first element: %i\n", lookup[0]);
    Rprintf("\t height: %i\n", offset[LOOKUPHEIGHT]);
    Rprintf("\t width: %i\n", offset[LOOKUPWIDTH]);
    Rprintf("\t expected size: %i\n", offset[LOOKUPHEIGHT] * offset[LOOKUPWIDTH]);
    Rprintf("\t actual size: %i\n", lookupSize);
    Rprintf("\t half way element: %i\n", lookup[lookupSize / 2 + 1]);
    Rprintf("\t last element: %i\n", lookup[(offset[LOOKUPHEIGHT] * offset[LOOKUPWIDTH]) - 10]);

    Rprintf("Data Matrix:\n");
    Rprintf("\t height: %i\n", data.height);
    Rprintf("\t width: %i\n", data.width);
    Rprintf("\t first element: %f\n", data.host[0]);
    Rprintf("\t last element: %f\n", data.host[(data.height * data.width) - 1]);
  }

  if (config.debug) {
    Rprintf("Running ellis\n");
  }

  int res = hostCalcEllis(config.mode, offset, lookup, data, *results, config.debug);

  if (config.debug) {
    Rprintf("Ellis result: %d\n", res);
  }

  if (config.debug > 1) {
    Rprintf("Results Matrix:\n");
    Rprintf("\theight: %i\n", results->height);
    Rprintf("\twidth: %i\n", results->width);
  }

  return res;
}

int runEllisWithFiles(const int *const offset, const size_t offsetSize) {
  int ret;
  int *lookup;
  DataMatrix data;
  ResultsMatrix results;
  size_t lookupSize = 0;

  if (config.debug) {
    Rprintf("Loading data from files\n");
  }

  ret = getLookup("data/simpleLU.csv", offset, &lookup, &lookupSize);
  if (ret != 0) {
    return ret;
  }

  ret = getDataMatrix("data/simpleDM.csv", &data);
  if (ret != 0) {
    free(lookup);
    return ret;
  }

  if (config.debug) {
    Rprintf("Data load complete\n");
  }

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


#ifdef WITHOUT_R
int main(int argc, char *argv[])
{
  char *evar;

  evar = secure_getenv("DEBUG");
  if (evar != NULL) {
    config.debug = atoi(evar);
    if (config.debug) {
      fprintf(stdout, "DEBUG=%s\n", config.debug ? "ON" : "OFF");
    }
  }

  evar = secure_getenv("MODE");
  if (evar != NULL) {
    config.mode = atoi(evar);
    if (config.debug) {
      fprintf(stdout, "MODE=%s\n", config.mode == 0 ? "NORMAL" : "TEST");
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
#else // WITHOUT_R
extern "C" {

void seeracEllis_setMode(SEXP r_mode) {
  config.mode = Rf_asInteger(r_mode);
}
SEXP seeracEllis_getMode() {
  return Rf_ScalarInteger(config.mode);
}

void seeracEllis_setDebug(SEXP r_debug) {
  config.debug = Rf_asInteger(r_debug);
}
SEXP seeracEllis_getDebug() {
  return Rf_ScalarInteger(config.debug);
}


SEXP seeracEllis_withFiles(SEXP r_offset) {
  const int *const offset = INTEGER(r_offset);
  int retVal = runEllisWithFiles(offset, LENGTH(r_offset));
  return Rf_ScalarInteger(retVal);
}

SEXP seeracEllis_withoutFiles(SEXP r_offset, SEXP r_lookup, SEXP r_data, SEXP r_results) {
  const int *const offset = INTEGER_RO(r_offset);
  const int *const lookup = INTEGER_RO(r_lookup);

  SEXP r_dim;

  DataMatrix data;
  r_dim = getAttrib(r_data, R_DimSymbol);
  data.width = INTEGER(r_dim)[0];
  data.height = INTEGER(r_dim)[1];
  data.host = REAL(r_data);

  ResultsMatrix results;
  r_dim = getAttrib(r_results, R_DimSymbol);
  results.width = INTEGER(r_dim)[0];
  results.height = INTEGER(r_dim)[1];
  results.host = REAL(r_results);

  int res = runEllis(offset, LENGTH(r_offset), lookup, LENGTH(r_lookup), data, &results);

  return Rf_ScalarInteger(res);
}
}
#endif // !WITHOUT_R