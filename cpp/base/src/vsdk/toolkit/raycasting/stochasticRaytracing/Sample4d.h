/**
4D vector sampling
*/

#ifndef SAMPLE_4D__
#define SAMPLE_4D__


#include "vsdk/toolkit/raycasting/stochasticRaytracing/Sampler4DSequence.h"

class Sample4d final {
  public:
    static void setSequence4D(Sampler4DSequence sequence);
    static double *sample4D(unsigned index);
    static void foldSampleU(unsigned *xi1, unsigned *xi2);
    static void foldSampleF(double *xi1, double *xi2);

  private:
    static Sampler4DSequence seq;
    static constexpr const char *RANDOM_NAME = "drand48";
    static constexpr const char *HALTON_NAME = "Halton";
    static constexpr const char *SCRAMBLED_HALTON_NAME = "ScramHalton";
    static constexpr const char *SOBOL_NAME = "sobol";
    static constexpr const char *ORIGINAL_FAURE_NAME = "faure";
    static constexpr const char *GENERALIZED_FAURE_NAME = "GFaure";
    static constexpr const char *NIEDERREITER_NAME = "Nied";
    static constexpr const char *UNKNOWN_NAME = "Unknown";

    static const char *sequenceName(Sampler4DSequence sequence);
};

#endif
