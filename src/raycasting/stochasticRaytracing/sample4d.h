/**
4D vector sampling
*/

#ifndef __SAMPLE_4D__
#define __SAMPLE_4D__

enum Sampler4DSequence {
    RANDOM,
    HALTON,
    SCRAMBLED_HALTON,
    SOBOL,
    ORIGINAL_FAURE,
    GENERALIZED_FAURE,
    NIEDERREITER
};

extern void setSequence4D(Sampler4DSequence sequence);
extern double *sample4D(unsigned index);
extern void foldSampleU(unsigned *, unsigned *);
extern void foldSampleF(double *, double *);

#endif
