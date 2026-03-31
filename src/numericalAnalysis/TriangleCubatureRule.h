#ifndef __TRIANGLE_CUBATURE_RULE__
#define __TRIANGLE_CUBATURE_RULE__

#include "numericalAnalysis/CubatureRule.h"

class TriangleCubatureRule {
  private:
    static CubatureRule crt1;
    static CubatureRule crt2;
    static CubatureRule crt3;
    static CubatureRule crt4;
    static CubatureRule crt5;
    static CubatureRule crt7;
    static CubatureRule crt9;

  public:
    static CubatureRule *degree8Rule();
    static void setTriangleCubatureRules(CubatureRule **triRule, CubatureDegree degree);
};

#endif
