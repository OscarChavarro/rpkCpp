/**
Sobol quasiMonteCarlo sequence
*/

#ifndef __SOBOL__
#define __SOBOL__

class Sobol {
  private:
    static double *nextSobol();
    static int sobolGray(int n);

  public:
    static double *sobol(int seed);
    static void initSobol(int iDim);
};

#endif
