#ifndef __SEERAC_ELLIS_R_H__
#define __SEERAC_ELLIS_R_H__

#ifndef WITHOUT_R

#include <R.h>
#include <Rinternals.h>

SEXP seeracEllis_getMaxGPUMem();

void seeracEllis_setMode(SEXP r_mode);
SEXP seeracEllis_getMode();

void seeracEllis_setDebug(SEXP r_debug);
SEXP seeracEllis_getDebug();

SEXP seeracEllis_withFiles(SEXP r_offset);
SEXP seeracEllis_withoutFiles(SEXP r_offset, SEXP r_lookup, SEXP r_data, SEXP r_results);

#endif // WITHOUT_R

#endif // __SEERAC_ELLIS_R_H__