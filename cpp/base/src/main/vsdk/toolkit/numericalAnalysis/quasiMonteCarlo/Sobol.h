/**
Sobol quasiMonteCarlo sequence
*/

#ifndef SOBOL__
#define SOBOL__

class Sobol {
  private:
    static constexpr int MAX_DIM = 5;
    static constexpr int V_MAX = 30;
    static int dim;
    static int nextN;
    static int x[MAX_DIM];
    static int v[MAX_DIM][V_MAX];
    static int skip;
    static double recip;

    static double *nextSobol();
    static int sobolGray(int n);

  public:
    static double *sobol(int seed);
    static void initSobol(int iDim);
};

#endif
