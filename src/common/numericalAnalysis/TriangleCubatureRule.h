#ifndef __TRIANGLE_CUBATURE_RULE__
#define __TRIANGLE_CUBATURE_RULE__

#include "common/numericalAnalysis/CubatureRule.h"

class TriangleCubatureRule {
  public:
    static void setTriangleCubatureRules(CubatureRule **triRule, CubatureDegree degree);
};

#endif
