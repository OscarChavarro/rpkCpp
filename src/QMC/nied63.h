/* nied63.h: Niederreiter QMC sample series (dimension 4, base 2, 
 * 63 bits, skip 4096) */

#ifndef _NIED63_H_
#define _NIED63_H_

#ifdef _NIED31_H_
#error "nied63.h and nied31.h cannot be included in the same source file"
#endif

#ifndef NOINT64
/* All this makes no sense if you don't have 64-bit integers */

#define SKIP 4096    /* number of samples to be skipped from the beginning of the
			 * series in order to deal with the "initial zeroes" phenomenon */
#define NBITS_POW  (1uLL<<NBITS)          /* 2^NBITS */
#define NBITS_POW1 (1uLL<<(NBITS-1))      /* 2^(NBITS-1) */

#define DIMEN 4        /* dimension of the samples generated. */
#define NBITS 63    /* number of bits in an integer, excluding the sign bit */
#define RECIP (1./9223372036854775808.)   /* 1 / 2^NBITS */
#define RECIP1 9223372036854775808.       /* 2^NBITS */

#define Nied Nied63
#define NextNiedInRange NextNiedInRange63
#define radicalInverse radicalInverse63
#define foldSample foldSample63
#define niedindex unsigned long long

extern unsigned long long *nied63(unsigned long long index);

extern unsigned long long *NextNiedInRange63(unsigned long long *idx, int dir,
                                             int nmsb,
                                             unsigned long long msb1,
                                             unsigned long long rmsb2);

extern unsigned long long radicalInverse63(unsigned long long n);

extern void foldSample63(unsigned long long *xi1, unsigned long long *xi2);

#endif /*NOINT64*/

#endif /*_NIEDERREITER_H_*/
