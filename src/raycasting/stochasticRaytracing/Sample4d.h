/**
4D vector sampling
*/

#ifndef __SAMPLE_4D__
#define __SAMPLE_4D__


#include "raycasting/stochasticRaytracing/Sampler4DSequence.h"

extern void setSequence4D(Sampler4DSequence sequence);
extern double *sample4D(unsigned index);
extern void foldSampleU(unsigned *, unsigned *);
extern void foldSampleF(double *, double *);

#endif
