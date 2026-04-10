#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

/**
Sobol quasiMonteCarlo sequence
*/

#ifndef __SOBOL__
#define __SOBOL__

#include "common/VSDK.h"

class Sobol {
  private:
    enum{
        MAX_DIM = 5,
        V_MAX = 30
    };
    static int dim;
    static int nextN;
    static int x[MAX_DIM];
    static int v[MAX_DIM][V_MAX];
    static int skip;
    static double recip;

    static double *nextSobol();
    static int sobolGray(int n);

  public:
    static double *sobol(int seed);
    static void initSobol(int iDim);
};

#endif
