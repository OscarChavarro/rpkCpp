#ifndef __STRATIFIED_SAMPLING_2D__
#define __STRATIFIED_SAMPLING_2D__

#include "common/VSDK.h"

/**
A simple 2D stratified sampling class. Only one sample per stratum. If the number
of samples does not fit a 2D grid, some samples are taken randomly over the
complete unit square.
*/
class StratifiedSampling2D {
  private:
    int xMaxStratum;
    int yMaxStratum;
    int xStratum;
    int yStratum;

    static void getNumberOfDivisions(int samples, int *divs1, int *divs2);

 public:
    explicit StratifiedSampling2D(int nrSamples);
    void sample(double *x1, double *x2);
};

#endif
