/**
Faure's quasiMonteCarlo sequences + generalized Faure sequences
*/

#ifndef __FAURE__
#define __FAURE__

extern double *faure(int seed);
extern void initOriginalFaureSequence(int iDim);
extern void initGeneralizedFaureSequence(int iDim);

#endif
