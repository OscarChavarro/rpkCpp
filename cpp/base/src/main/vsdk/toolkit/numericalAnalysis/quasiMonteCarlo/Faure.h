/**
Faure's quasiMonteCarlo sequences + generalized Faure sequences
*/

#ifndef FAURE__
#define FAURE__

#include "vsdk/toolkit/numericalAnalysis/quasiMonteCarlo/FaureSequenceLimits.h"

class Faure {
  private:
    static constexpr int prime[FaureSequenceLimits::MAX_DIMENSION] = {2, 3, 5, 5, 7, 7, 11, 11, 11, 11};
    static int ix[FaureSequenceLimits::MAX_DIMENSION][FaureSequenceLimits::MAX_PRIME_DIGITS];
    static int dimension;
    static int primeBase;
    static int nextN;
    static int skip;
    static int nDigits;
    static int generatorMatrix[FaureSequenceLimits::MAX_DIMENSION][FaureSequenceLimits::MAX_PRIME_DIGITS][FaureSequenceLimits::MAX_PRIME_DIGITS];

    static int setFaureC();
    static int setGFaureC();
    static double *nextFaure();

  public:
    static double *faure(int seed);
    static void initOriginalFaureSequence(int iDim);
    static void initGeneralizedFaureSequence(int iDim);
};

#endif
