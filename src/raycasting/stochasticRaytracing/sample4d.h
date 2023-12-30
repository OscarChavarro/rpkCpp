/* sample4d.h: 4D vector sampling */

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

/* also initialises the sequence */
extern void SetSequence4D(SEQ4D sequence);

/* returns 4D sample with given index from current sequence. When the
 * current sequence is 'random', the index is not used. */
extern double *Sample4D(unsigned index);

/* The following routines are safe with Sample4D(), which calls only
 * 31-bit sequences (including 31-bit Niederreiter sequence). If
 * you are looking for such a routine to use directly in conjunction
 * with the routined Nied() or NextNiedInRange(), you should use
 * the FoldSample() routine in niederreiter.h instead.
 * Nied() and NextNiedInRange() are 63-bit unless compiled without
 * 'unsigned long long' support. */
extern void FoldSampleU(unsigned *, unsigned *);

extern void FoldSampleF(double *, double *);

#endif /*_SAMPLE_4D_*/
