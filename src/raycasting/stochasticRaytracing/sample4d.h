/**
4D vector sampling
*/

#ifndef __SAMPLE_4D__
#define __SAMPLE_4D__

enum SEQ4D {
    S4D_RANDOM,
    S4D_HALTON,
    S4D_SCRAMBLED_HALTON,
    S4D_SOBOL,
    S4D_ORIGINAL_FAURE,
    S4D_GENERALIZED_FAURE,
    S4D_NIEDERREITER
};

extern void setSequence4D(SEQ4D sequence);
extern double *sample4D(unsigned index);
extern void foldSampleU(unsigned *, unsigned *);
extern void foldSampleF(double *, double *);

#endif
