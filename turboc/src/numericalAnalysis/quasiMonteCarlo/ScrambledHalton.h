#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

/**
Scrambled Halton quasiMonteCarlo sequence
*/

#ifndef __SCRAMBLED_HALTON__
#define __SCRAMBLED_HALTON__

#include "common/VSDK.h"

class ScrambledHalton {
  private:
    enum{
        MAX_DIM = 10
    };
    static const int prime[MAX_DIM];

  public:
    static double *scrambledHalton(unsigned nextN, int dim);
};

#endif
