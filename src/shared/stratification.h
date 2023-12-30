/* stratification.H : aids simple stratified sampling. */

#ifndef _STRATIFICATION_H_
#define _STRATIFICATION_H_

/* A simple 2D stratified sampling class. Only one sample
   per stratum ! If the number of samples does not fit a
   2D grid, some samples are taken randomly over the
   complete unit square */

class CStrat2D {
    int totalSamples;
    int currentSample;
    int xMaxStratum, yMaxStratum;
    int xStratum, yStratum;

    // methods
public:
    CStrat2D(int nrSamples);

    void Init();

    void Sample(double *x_1, double *x_2);
};

#endif
