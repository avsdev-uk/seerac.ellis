#' seerac.ellis: Benthic Recovery Algorithm for the SEERAC Project
#'
#' Benthic recovery algorithm for the SEERAC project utilising GPU CUDA. Based
#' off the Ellis et al paper.
#'
#' @section Mypackage functions:
#' The mypackage functions ...
#'
#' @docType package
#' @name seerac.ellis
#' @useDynLib seerac_ellis, .registration = TRUE
NULL
#> NULL

#' Get the total memory capacity of the GPU
#'
#' @return The maximum memory of the GPU, NULL on failure
#' @export
getMaxGPUMemory <- function() {
  maxMem <- .Call(seeracEllis_getMaxGPUMem)
  if (maxMem < 0) {
    return(NULL)
  }
  return(maxMem)
}

#' Turns on test mode
#'
#' @export
testModeOn <- function() {
  setMode(1)
}
#' Turns off test mode
#'
#' @export
testModeOff <- function() {
  setMode(0)
}
#' Set test mode on or off
#'
#' @param mode integer, 0 for normal, 1 for test.
#' @export
setMode <- function(mode) {
  .Call(seeracEllis_setMode, as.integer(mode))
  invisible(0)
}
#' Get the current mode
#'
#' @return The current mode. 0 for normal, 1 for test.
#' @export
getMode <- function() {
  .Call(seeracEllis_getMode)
}

#' Turns on basic debug messaging
#'
#' @export
debugOn <- function() {
  setDebug(1)
}
#' Turns off basic debug messaging
#'
#' @export
debugOff <- function() {
  setDebug(0)
}

#' Set debug messaging level
#'
#' @param debug integer, 0 for off, 1 for basic, 2 for advanced
#' @export
setDebug <- function(debug) {
  .Call(seeracEllis_setDebug, as.integer(debug))
  invisible(0)
}
#' Get the current debug messaging level
#'
#' @return The current debug messaging level, 0 for off, 1 for basic, 2 for advanced.
#' @export
getDebug <- function() {
  .Call(seeracEllis_getDebug)
}

#' Emulate the original implementation of the Benthic Recovery algorithm which saved all the input
#' data to files and then called a compiled executable. Assumes the caller has generated the input
#' files required. Used for testing purposes only.
#'
#' @param offsets integer vector. The indexing parameters of the data and lookup blocks.
#' @return The result of the algorithm. 0 for success, negative number for error
#' @export
seerac.ellisWithFiles <- function(offsets) {
  .Call(seeracEllis_withFiles, as.integer(offsets))
}

#' Wrapper for passing in lookups and data into the compiled CUDA implementation of the Benthic
#' Recovery algorithm.
#'
#' @param offsets integer vector. The indexing parameters of the data and lookup blocks.
#' @param lookup integer matrix. The lookup table which is loaded into GPU const memory.
#' @param data float matrix. Input data table which is used to produce an output results matrix
#' @return NULL if an error occurred, A float matrix containing the results.
#' @export
seerac.ellis <- function(offsets, lookup, data) {
  results <- matrix(
    data = -2,
    nrow = offsets$numPeriods * offsets$numSpecies * 6,
    ncol = nrow(data),
    dimnames = list(NULL, NULL)
  )

  ret <- .Call(
    seeracEllis_withoutFiles,
    as.integer(offsets),
    as.integer(t(lookup)),
    t(data),
    results
  )
  if (ret != 0) {
    warning("seerac.ellis failed to compute")
    return(NULL)
  }

  return(t(results))
}