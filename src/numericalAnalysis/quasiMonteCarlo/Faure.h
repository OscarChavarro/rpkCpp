/**
Faure's quasiMonteCarlo sequences + generalized Faure sequences
*/

#ifndef __FAURE__
#define __FAURE__

class Faure {
  private:
    static int setFaureC();
    static int setGFaureC();
    static double *nextFaure();

  public:
    static double *faure(int seed);
    static void initOriginalFaureSequence(int iDim);
    static void initGeneralizedFaureSequence(int iDim);
};

#endif
