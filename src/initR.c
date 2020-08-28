#include "seerac-ellis_R.h"

#ifndef WITHOUT_R

static const R_CallMethodDef callMethods[] = {
   {"seeracEllis_getMaxGPUMem", (DL_FUNC) &seeracEllis_getMaxGPUMem, 0},
   {"seeracEllis_setDebug", (DL_FUNC) &seeracEllis_setDebug, 1},
   {"seeracEllis_getDebug", (DL_FUNC) &seeracEllis_getDebug, 0},
   {"seeracEllis_setMode", (DL_FUNC) &seeracEllis_setMode, 1},
   {"seeracEllis_getMode", (DL_FUNC) &seeracEllis_getMode, 0},
   {"seeracEllis_withFiles", (DL_FUNC) &seeracEllis_withFiles, 1},
   {"seeracEllis_withoutFiles", (DL_FUNC) &seeracEllis_withoutFiles, 4},
   {NULL, NULL, 0}
};

void R_init_seerac_ellis(DllInfo *dll)
{
  R_registerRoutines(dll, NULL, callMethods, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
  R_forceSymbols(dll, TRUE);
}

#endif
#ifdef WITHOUT_R
typedef int make_iso_compilers_happy;
#endif