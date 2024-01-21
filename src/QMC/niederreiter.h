/* niederreiter.h: Niederreiter QMC sample series (dimension 4, base 2, 
 * 31 or 63 bits, skip 4096) */

#ifndef _NIEDERREITER_H_
#define _NIEDERREITER_H_

#ifndef NOINT64
/* use 63bit sequence by default */
#include "QMC/nied63.h"

#else
/* use 31bit sequence */
#include "QMC/nied31.h"
#endif

/*
 * The header files above define niedindex as either 'unsigned' or 
 * 'unsigned long long', depending on whether you have
 * integers on your system or not 
 *
 * The header files above also define the following constants:
 * DIMEN   4		dimension of the samples generated.
 * NBITS   63 or 31     number of bits in an integer, excluding the sign bit
 * RECIP   1 / 2^NBITS  multiply niedindex values by this to get 
 *                      floating point values in the range 0..1
 * RECIP1  2^NBITS      multiply floating point values in the range 0..1 
 *                      by this factor in order to convert to niedindex
 * NBITS_POW  (1<<NBITS)       2^NBITS niedindex value
 * NBITS_POW1 (1<<(NBITS-1))   2^(NBITS-1) niedindex value
 */

/* Computes the base-2 NBITS-bits 4D Niederreiter sample with index n.
 * contrary to the literature, the index is not transformed to its Gray
 * code. The routine still is very efficient for computing subsequent
 * samples. An array of four NBITS-bits integers is returned. Multiply
 * with RECIP in order to obtain floating point numbers between 0 and 1.
k * Do not modify the returned ints. */
extern niedindex *Nied(niedindex n);

/* Finds the next Niederreiter sample following or preceeding the
 * sample with index *idx with first 2 components in a range defined by
 * nmsb, msb1, rmsb2:
 * - if dir>0, the next sample (including the current one) in the range is 
 *   	determined, 
 *   if dir<0, the previous sample (including the current sample) is found.
 *   The current sample is the sample with index *idx at entry.
 * - nmsb contains how many bits of msb1 and rmsb2 are significant in
 *   the definition of the range. 
 * - the routine looks for samples with 1st components having the same nmsb 
 *   most significant bits as the base-2 radical inverse of msb1.
 * - the 2nd component has the same nmsb most significant bits as msb2.
 * Note the difference between the specification of the most significant bits
 * with msb1 and msb2. This is so because of efficiency reasons (which are
 * very important for this routine!).
 * Upon exit, *idx will contain the index of the sample that is returned. */
extern niedindex *NextNiedInRange(niedindex *idx, int dir,
                                  int nmsb,
                                  niedindex msb1,
                                  niedindex rmsb2);

/* Computes the (NBITS-bits) base-2 radical inverse of the given number */
extern niedindex radicalInverse(niedindex n);

/* "folds" a sample in the unit square to the standard triangle (0,0),(1,0),(0,1) */
extern void foldSample(niedindex *xi1, niedindex *xi2);

#endif
