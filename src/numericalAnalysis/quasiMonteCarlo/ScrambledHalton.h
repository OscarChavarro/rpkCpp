/**
Scrambled Halton quasiMonteCarlo sequence
*/

#ifndef __SCRAMBLED_HALTON__
#define __SCRAMBLED_HALTON__

class ScrambledHalton {
  private:
    static constexpr int MAX_DIM = 10;
    static const int prime[MAX_DIM];

  public:
    static double *scrambledHalton(unsigned nextN, int dim);
};

#endif
