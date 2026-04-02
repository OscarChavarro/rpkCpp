/**
Niederreiter quasiMonteCarlo sample series (dimension 4, base 2, 31 bits, skip 4096)
*/

#ifndef __NIED_31__
#define __NIED_31__

#ifdef __NIED63__
    #error "Niederreiter63.h and Niederreiter31.h cannot be included in the same source file"
#endif

using NiederreiterIndex = unsigned;

#include "numericalAnalysis/quasiMonteCarlo/NiederreiterCore.txx"

class Niederreiter31 {
  public:
    // Number of samples to be skipped from the beginning of the series in order to
    // deal with the "initial zeroes" phenomenon
    static constexpr unsigned SKIP = 4096;

    // Dimension of the samples generated
    static constexpr unsigned DIMEN = 4;

    // Number of bits in an integer, excluding the sign bit
    static constexpr unsigned NBITS = 31;

    // 1/2^NBITS
    static constexpr double RECIP = 1.0 / 2147483648.0;

    // 2^NBITS
    static constexpr double RECIP1 = 2147483648.0;

    // 2^NBITS
    static constexpr unsigned NBITS_POW = (1u << NBITS);

    // 2^(NBITS-1)
    static constexpr unsigned NBITS_POW1 = (1u << (NBITS - 1));

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
