/**
Aids simple stratified sampling
*/

#ifndef __STRATIFICATION__
#define __STRATIFICATION__

/**
A simple 2D stratified sampling class. Only one sample
per stratum ! If the number of samples does not fit a
2D grid, some samples are taken randomly over the
complete unit square
*/
class StratifiedSampling2D {
    int xMaxStratum;
    int yMaxStratum;
    int xStratum;
    int yStratum;

public:
    explicit StratifiedSampling2D(int nrSamples);
    void sample(double *x1, double *x2);
};

extern void getNumberOfDivisions(int samples, int *divs1, int *divs2);

#endif
