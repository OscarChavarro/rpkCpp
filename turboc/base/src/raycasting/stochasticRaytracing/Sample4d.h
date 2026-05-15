#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

/**
4D vector sampling
*/

#ifndef __SAMPLE_4D__
#define __SAMPLE_4D__

#include "raycasting/stochasticRaytracing/Sampler4DSequence.h"

class Sample4d{ public:
    static void setSequence4D(Sampler4DSequence sequence);
    static double *sample4D(unsigned index);
    static void foldSampleU(unsigned *xi1, unsigned *xi2);
    static void foldSampleF(double *xi1, double *xi2);

  private:
    static Sampler4DSequence seq;
    static const char *RANDOM_NAME;
    static const char *HALTON_NAME;
    static const char *SCRAMBLED_HALTON_NAME;
    static const char *SOBOL_NAME;
    static const char *ORIGINAL_FAURE_NAME;
    static const char *GENERALIZED_FAURE_NAME;
    static const char *NIEDERREITER_NAME;
    static const char *UNKNOWN_NAME;

    static const char *sequenceName(Sampler4DSequence sequence);
};

#endif
