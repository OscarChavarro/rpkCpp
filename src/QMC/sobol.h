/**
Sobol QMC sequence
*/

#ifndef __SOBOL__
#define __SOBOL__

extern double *nextSobol();
extern double *sobol(int seed);
extern void initSobol(int iDim);

#endif

