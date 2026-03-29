/**
4D vector sampling
*/

#ifndef __SAMPLE_4D__
#define __SAMPLE_4D__


#include "raycasting/stochasticRaytracing/Sampler4DSequence.h"

class Sample4d final {
  public:
    static void setSequence4D(Sampler4DSequence sequence);
    static double *sample4D(unsigned index);
    static void foldSampleU(unsigned *xi1, unsigned *xi2);
    static void foldSampleF(double *xi1, double *xi2);

  private:
    static const char *sequenceName(Sampler4DSequence sequence);
};

#endif
