#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

/**
Niederreiter quasiMonteCarlo sample series (dimension 4, base 2, 31 bits, skip 4096)
*/

#ifndef __NIED_31__
#define __NIED_31__

#include "common/VSDK.h"

typedef unsigned NiederreiterIndex;

#include "numericalAnalysis/quasiMonteCarlo/NiederreiterCore.txx"

class Niederreiter31 {
  public:
    // Number of samples to be skipped from the beginning of the series in order to
    // deal with the "initial zeroes" phenomenon
    enum{
        SKIP = 4096,
        DIMEN = 4,
        NBITS = 31,
        NBITS_POW = (1u << NBITS),
        NBITS_POW1 = (1u << (NBITS - 1))
    };
    static const double RECIP;
    static const double RECIP1;

  private:
    static const unsigned directionNumbers[DIMEN][NBITS];
    static NiederreiterCore<unsigned, DIMEN, NBITS> core;

  public:
    static unsigned *niederreiter31(unsigned index);

    static unsigned *
    NextNiedInRange31(
        unsigned *idx,
        int dir,
        int nmsb,
        unsigned msb1,
        unsigned rmsb2);

    static unsigned radicalInverse31(unsigned n);

    static void foldSample31(unsigned *xi1, unsigned *xi2);
};

#endif
