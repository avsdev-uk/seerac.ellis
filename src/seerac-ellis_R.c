#ifndef WITHOUT_R

#include <R.h>
#include <Rdefines.h>
#include <Rinternals.h>

#include "seerac-ellis.h"
#include "seerac-ellis_R.h"


void seeracEllis_setMode(SEXP r_mode) {
  setMode(Rf_asInteger(r_mode));
}
SEXP seeracEllis_getMode() {
  return Rf_ScalarInteger(getMode());
}

void seeracEllis_setDebug(SEXP r_debug) {
  setDebug(Rf_asInteger(r_debug));
}
SEXP seeracEllis_getDebug() {
  return Rf_ScalarInteger(getDebug());
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
#endif // WITHOUT_R
#ifdef WITHOUT_R
typedef int make_iso_compilers_happy;
#endif