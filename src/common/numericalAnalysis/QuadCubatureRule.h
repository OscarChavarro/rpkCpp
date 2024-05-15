#ifndef __QUAD_CUBATURE_RULE_H
#define __QUAD_CUBATURE_RULE_H

#include "common/numericalAnalysis/CubatureRule.h"

extern CubatureRule GLOBAL_crq8; // quads, degree 8, 16 nodes
extern CubatureRule GLOBAL_crt8; // triangles, degree 8, 16 nodes
extern CubatureRule GLOBAL_crv1; // boxes, degree 1,  8 nodes (the corners)

class QuadCubatureRule {
  public:
    static void fixCubatureRules();
    static void setQuadCubatureRules(CubatureRule **quadRule, CubatureDegree degree);
};

#endif
