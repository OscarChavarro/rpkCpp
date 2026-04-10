#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __QUAD_CUBATURE_RULE_H
#define __QUAD_CUBATURE_RULE_H

#include "numericalAnalysis/CubatureRule.h"

class QuadCubatureRule {
  private:
    #define D_2_3_W 4.0 / 3.0
    #define D_2_3_U 0.81649658092772603272
    #define D_2_3_C -0.5
    #define D_2_3_S 0.86602540378443864676

    #define D_3_4_U 0.81649658092772603272
    #define D_3_4_G_U 0.57735026918962576450

    #define D_5_7_W1 8.0 / 7.0
    #define D_5_7_W2 5.0 / 9.0
    #define D_5_7_W3 20.0 / 63.0
    #define D_5_7_R 0.96609178307929588492
    #define D_5_7_S 0.57735026918962573106
    #define D_5_7_T 0.77459666924148340428

    #define D_5_9_X0 0.0
    #define D_5_9_W0 8.0 / 9.0
    #define D_5_9_X1 0.7745966692414834
    #define D_5_9_W1 5.0 / 9.0

    #define D_7_12_R 0.92582009977255141919
    #define D_7_12_S 0.38055443320831561227
    #define D_7_12_T 0.80597978291859884159
    #define D_7_12_W1 0.24197530864197530631
    #define D_7_12_W2 0.52059291666739448967
    #define D_7_12_W3 0.23743177469063023177

    #define D_7_16_X1 0.86113631159405257522
    #define D_7_16_X2 0.33998104358485626480
    #define D_7_16_W1 0.34785484513745385737
    #define D_7_16_W2 0.65214515486254614263

    #define D_9_17_B1 0.96884996636197772072
    #define D_9_17_B2 0.75027709997890053354
    #define D_9_17_B3 0.52373582021442933604
    #define D_9_17_B4 0.07620832819261717318
    #define D_9_17_C1 0.63068011973166885417
    #define D_9_17_C2 0.92796164595956966740
    #define D_9_17_C3 0.45333982113564719076
    #define D_9_17_C4 0.85261572933366230775
    #define D_9_17_W0 0.52674897119341563786
    #define D_9_17_W1 0.08887937817019870697
    #define D_9_17_W2 0.11209960212959648528
    #define D_9_17_W3 0.39828243926207009528
    #define D_9_17_W4 0.26905133763978080301

    #define D_1_9_U 1.0
    #define D_1_9_W 8.0 / 9.0
    #define D_3_8_U 0.57735026918962576450

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

    static void ensureRulesTransformed();
    static void transformQuadRuleInPlace(CubatureRule *rule);
    static void transformCubeRuleInPlace(CubatureRule *rule);
    static CubatureRule createTransformedQuadRule(CubatureRule rule);
    static CubatureRule createTransformedCubeRule(CubatureRule rule);

  public:
    static CubatureRule *degree8QuadrilateralRule();
    static CubatureRule *degree1BoxRule();
    static void setQuadCubatureRules(CubatureRule **quadRule, CubatureDegree degree);
};

#endif
