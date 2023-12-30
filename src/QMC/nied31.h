/* niederreiter.h: Niederreiter QMC sample series (dimension 4, base 2, 
 * 31 bits, skip 4096) */

#ifndef _NIED31_H_
#define _NIED31_H_

#ifdef _NIED63_H_
#error "nied63.h and nied31.h cannot be included in the same source file"
#endif

#define SKIP 4096    /* number of samples to be skipped from the beginning of the
			 * series in order to deal with the "initial zeroes" phenomenon */
#define DIMEN 4        /* dimension of the samples generated. */
#define NBITS 31    /* number of bits in an integer, excluding the sign bit */
#define RECIP (1./2147483648.)    /* 1/2^NBITS */
#define RECIP1 2147483648.    /* 2^NBITS */

#define NBITS_POW  (1u<<NBITS)          /* 2^NBITS */
#define NBITS_POW1 (1u<<(NBITS-1))      /* 2^(NBITS-1) */

#define Nied Nied31
#define NextNiedInRange NextNiedInRange31
#define RadicalInverse RadicalInverse31
#define FoldSample FoldSample31
#define niedindex unsigned

extern unsigned *Nied31(unsigned index);

extern unsigned *NextNiedInRange31(unsigned *idx, int dir,
                                   int nmsb,
                                   unsigned msb1,
                                   unsigned rmsb2);

extern unsigned RadicalInverse31(unsigned n);

extern void FoldSample31(unsigned *xi1, unsigned *xi2);

#endif
