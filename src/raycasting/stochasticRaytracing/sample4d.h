/**
4D vector sampling
*/

#ifndef _SAMPLE_4D_
#define _SAMPLE_4D_

enum SEQ4D {
    S4D_RANDOM,
    S4D_HALTON,
    S4D_SCRAMHALTON,
    S4D_SOBOL,
    S4D_FAURE,
    S4D_GFAURE,
    S4D_NIEDERREITER
};

#define SEQ4D_NAME(seq) (\
(seq == S4D_RANDOM) ? "drand48" : (\
(seq == S4D_HALTON) ? "Halton" : (\
(seq == S4D_SCRAMHALTON) ? "ScramHalton" : (\
(seq == S4D_SOBOL) ? "Sobol" : (\
(seq == S4D_FAURE) ? "Faure" : (\
(seq == S4D_GFAURE) ? "GFaure" : (\
(seq == S4D_NIEDERREITER) ? "Nied" : "Unknown"\
)))))))

extern void setSequence4D(SEQ4D sequence);
extern double *sample4D(unsigned index);
extern void foldSampleU(unsigned *, unsigned *);
extern void foldSampleF(double *, double *);

#endif
