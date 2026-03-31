#ifndef __QUAD_CUBATURE_RULE_H
#define __QUAD_CUBATURE_RULE_H

#include "numericalAnalysis/CubatureRule.h"

class QuadCubatureRule {
  private:
    static CubatureRule crq1;
    static CubatureRule crq2;
    static CubatureRule crq3;
    static CubatureRule crq3Pg;
    static CubatureRule crq4;
    static CubatureRule crq5;
    static CubatureRule crq5Pg;
    static CubatureRule crq6;
    static CubatureRule crq7;
    static CubatureRule crq7Pg;
    static CubatureRule crq9;
    static CubatureRule crv3Pg;

    static CubatureRule *const quadProductRule[3];
    static CubatureRule *const boxesProductRule[1];

  public:
    static CubatureRule *degree8QuadrilateralRule();
    static CubatureRule *degree1BoxRule();
    static void setQuadCubatureRules(CubatureRule **quadRule, CubatureDegree degree);
};

#endif
