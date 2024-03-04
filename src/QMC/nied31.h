/**
Niederreiter QMC sample series (dimension 4, base 2, 31 bits, skip 4096)
*/

#ifndef __NIED_31__
#define __NIED_31__

#ifdef __NIED63__
    #error "nied63.h and nied31.h cannot be included in the same source file"
#endif

// Number of samples to be skipped from the beginning of the series in order to
// deal with the "initial zeroes" phenomenon
#define SKIP 4096

// Dimension of the samples generated
#define DIMEN 4

// Number of bits in an integer, excluding the sign bit
#define NBITS 31

// 1/2^NBITS
#define RECIP (1.0/2147483648.0)

// 2^NBITS
#define RECIP1 2147483648.0

// 2^NBITS
#define NBITS_POW (1u << NBITS)

// 2^(NBITS-1)
#define NBITS_POW1 (1u << (NBITS - 1))

#define Nied Nied31
#define NextNiedInRange NextNiedInRange31
#define radicalInverse radicalInverse31
#define foldSample foldSample31
#define niedindex unsigned

extern unsigned *Nied31(unsigned index);

extern unsigned *
NextNiedInRange31(
    unsigned *idx,
    int dir,
    int nmsb,
    unsigned msb1,
    unsigned rmsb2);

extern unsigned radicalInverse31(unsigned n);

extern void foldSample31(unsigned *xi1, unsigned *xi2);

#endif
