#include "common/logging/Logger.h"
#include "numericalAnalysis/QuadCubatureRule.h"

void
QuadCubatureRule::transformQuadRuleInPlace(CubatureRule *rule) {
    if ( rule == NULL ) {
        return;
    }
    for ( int k = 0; k < rule->numberOfNodes; k++ ) {
        rule->u[k] = (rule->u[k] + 1.0) / 2.0;
        rule->v[k] = (rule->v[k] + 1.0) / 2.0;
        rule->w[k] /= 4.0;
    }
}

void
QuadCubatureRule::transformCubeRuleInPlace(CubatureRule *rule) {
    if ( rule == NULL ) {
        return;
    }
    for ( int k = 0; k < rule->numberOfNodes; k++ ) {
        rule->u[k] = (rule->u[k] + 1.0) / 2.0;
        rule->v[k] = (rule->v[k] + 1.0) / 2.0;
        rule->t[k] = (rule->t[k] + 1.0) / 2.0;
        rule->w[k] /= 8.0;
    }
}

CubatureRule
QuadCubatureRule::createTransformedQuadRule(CubatureRule rule) {
    QuadCubatureRule::transformQuadRuleInPlace(&rule);
    return rule;
}

CubatureRule
QuadCubatureRule::createTransformedCubeRule(CubatureRule rule) {
    QuadCubatureRule::transformCubeRuleInPlace(&rule);
    return rule;
}

void
QuadCubatureRule::ensureRulesTransformed() {
    static bool transformed = false;
    if ( transformed ) {
        return;
    }

    transformQuadRuleInPlace(&crq1);
    transformQuadRuleInPlace(&crq2);
    transformQuadRuleInPlace(&crq3);
    transformQuadRuleInPlace(&crq3Pg);
    transformQuadRuleInPlace(&crq4);
    transformQuadRuleInPlace(&crq5);
    transformQuadRuleInPlace(&crq5Pg);
    transformQuadRuleInPlace(&crq6);
    transformQuadRuleInPlace(&crq7);
    transformQuadRuleInPlace(&crq7Pg);
    transformQuadRuleInPlace(&crq9);
    transformCubeRuleInPlace(&crv3Pg);

    transformed = true;
}

/**
quadrilateral rules are specified below in the canonical [-1, 1]^2 domain and
pretransformed during static initialization to [0, 1]^2.
*/

/**
Degree 1, 1 point
*/
CubatureRule QuadCubatureRule::crq1 = {
    "quads degree 1, 1 point",
    1,
    {0.0},
    {0.0},
    {0.0},
    {4.0}
};

/**
Degree 2, 3 positions Stroud '71
*/
CubatureRule QuadCubatureRule::crq2 = {
    "quads degree 2, 3 positions",
    3,
    {D_2_3_U, D_2_3_U * D_2_3_C, D_2_3_U * D_2_3_C},
    {0.0, D_2_3_U * D_2_3_S, -D_2_3_U * D_2_3_S},
    {0.0, 0.0, 0.0},
    {D_2_3_W, D_2_3_W, D_2_3_W}
};

/**
Degree 3, 4 positions, Davis & Rabinowitz, Methods of Numerical Integration,
2nd edition 1984, p 367
*/
CubatureRule QuadCubatureRule::crq3 = {
    "quads degree 3, 4 positions",
    4,
    {D_3_4_U, 0.0, -D_3_4_U, 0.0},
    {0.0, D_3_4_U, 0.0, -D_3_4_U},
    {0.0, 0.0, 0.0, 0.0},
    {1.0, 1.0, 1.0, 1.0}
};

// Degree 3, 4 positions, product Gauss-Legendre formula
// sqrt(1/3)
CubatureRule QuadCubatureRule::crq3Pg = {
    "quads degree 3, 4 positions, product Gauss formula",
    4,
    {D_3_4_G_U, D_3_4_G_U, -D_3_4_G_U, -D_3_4_G_U}, // 1st coord. of abscissa
    {D_3_4_G_U, -D_3_4_G_U, D_3_4_G_U, -D_3_4_G_U}, // 2nd coord. of abscissa
    {0.0, 0.0, 0.0, 0.0}, // 3rd coord. of abscissa
    {1.0, 1.0, 1.0, 1.0} // Weights
};

/**
Degree 4, 6 positions
see: Wissman, Becker, "Partially Symmetric Cubature Formulas for Even
Degrees of Exactness", SIAM. J. Numer. Anal., Vol 23 nr 3 (1986), p 676
You'll find also another similar rule in this paper, but I chose this one
because the abscissa seem to be nicer located.
You'll find the same rule in: Schmid, "On Cubature Formulae with a Minimal
Number of Knots", Numer. Math. Vol 31 (1978) p281
*/
CubatureRule QuadCubatureRule::crq4 = {
    "quads degree 4, 6 positions",
    6,
    {0.0, 0.0, 0.774596669241483, -0.774596669241483, 0.774596669241483, -0.774596669241483},
    {-0.356822089773090, 0.934172358962716, 0.390885162530071, 0.390885162530071, -0.852765377881771, -0.852765377881771},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {1.286412084888852, 0.491365692888926, 0.761883709085613, 0.761883709085613, 0.349227402025498, 0.349227402025498}
};

// Degree 5, 7 positions, Radon's rule see e.g. Stroud '71
CubatureRule QuadCubatureRule::crq5 = {
    "quads degree 5, 7 positions, Radon's rule",
    7,
    {0.0, D_5_7_S, D_5_7_S, -D_5_7_S, -D_5_7_S, D_5_7_R, -D_5_7_R},
    {0.0, D_5_7_T, -D_5_7_T, D_5_7_T, -D_5_7_T, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {D_5_7_W1, D_5_7_W2, D_5_7_W2, D_5_7_W2, D_5_7_W2, D_5_7_W3, D_5_7_W3}
};

// Degree 5, 9 positions product Gauss-Legendre rule
// abscissa and weights computed using Stuff/gauleg.c
CubatureRule QuadCubatureRule::crq5Pg = {
    "quads degree 5, 9 positions product Gauss rule",
    9,
    {-D_5_9_X1, -D_5_9_X1, -D_5_9_X1, D_5_9_X0, D_5_9_X0, D_5_9_X0, D_5_9_X1, D_5_9_X1, D_5_9_X1},
    {-D_5_9_X1, D_5_9_X0, D_5_9_X1, -D_5_9_X1, D_5_9_X0, D_5_9_X1, -D_5_9_X1, D_5_9_X0, D_5_9_X1},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {D_5_9_W1 * D_5_9_W1, D_5_9_W1 * D_5_9_W0, D_5_9_W1 * D_5_9_W1, D_5_9_W0 * D_5_9_W1,
     D_5_9_W0 * D_5_9_W0, D_5_9_W0 * D_5_9_W1, D_5_9_W1 * D_5_9_W1, D_5_9_W1 * D_5_9_W0, D_5_9_W1 * D_5_9_W1}
};

/**
Degree 6, 10 positions
from: Wissmann & Becker (cfr supra)
They again give two formulae of this type and you'll also find one
in Schmid, but I chose this one because it has the nicest weights
*/
CubatureRule QuadCubatureRule::crq6 = {
    "quads degree 6, 10 positions",
    10,
    {0.0, 0.0, 0.863742826346154, -0.863742826346154,
     0.518690521392592, -0.518690521392592, 0.933972544972849, -0.933972544972849,
     0.608977536016356, -0.608977536016356},
    {0.869833375250059, -0.479406351612111, 0.802837516207657, 0.802837516207657,
     0.262143665508058, 0.262143665508058, -0.363096583148066, -0.363096583148066,
     -0.896608632762453, -0.896608632762453},
    {0.0, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0,
     0.0, 0.0},
    {0.392750590964348, 0.754762881242610, 0.206166050588279, 0.206166050588279,
     0.689992138489864, 0.689992138489864, 0.260517488732317, 0.260517488732317,
     0.269567586086061, 0.269567586086061}
};

/**
Degree 7, 12 positions
from: Stroud, "Approximate Calculation of Multiple Integrals", 1971
This is just one of many similar rules (see Cools & Rabinowitz, "Monomial
Cubature rules since "Stroud": A Compilation", J. Comp. Appl. Math. 48
(1993) 309-326).

Moeller, "Kubaturformeln mit minimaler Knotenzahl", Numer. Math. 25 (1976)
p 185 presents a generalisation of this formula if you would need something
else. There a program in Stuff/moeller.c to compute the nodes.

I don't think the other rules will be better than this one (Haegemans & Piessens,
SIAM J. Numer Anal 14 (1977) p 492 is maybe a nice alternative? Other formulas have
less symmetry.
*/
CubatureRule QuadCubatureRule::crq7 = {
    "quads degree 7, 12 positions",
    12,
    {D_7_12_R, -D_7_12_R, 0.0, 0.0, D_7_12_S, D_7_12_S, -D_7_12_S, -D_7_12_S, D_7_12_T, D_7_12_T, -D_7_12_T, -D_7_12_T},
    {0.0, 0.0, D_7_12_R, -D_7_12_R, D_7_12_S, -D_7_12_S, D_7_12_S, -D_7_12_S, D_7_12_T, -D_7_12_T, D_7_12_T, -D_7_12_T},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {D_7_12_W1, D_7_12_W1, D_7_12_W1, D_7_12_W1,
     D_7_12_W2, D_7_12_W2, D_7_12_W2, D_7_12_W2,
     D_7_12_W3, D_7_12_W3, D_7_12_W3, D_7_12_W3}
};

// Degree 7, 16 positions product Gauss rule
CubatureRule QuadCubatureRule::crq7Pg = {
    "quads degree 7, 16 positions product Gauss rule",
    16,
    {-D_7_16_X1, -D_7_16_X1, -D_7_16_X1, -D_7_16_X1, -D_7_16_X2, -D_7_16_X2, -D_7_16_X2, -D_7_16_X2,
     D_7_16_X2, D_7_16_X2, D_7_16_X2, D_7_16_X2, D_7_16_X1, D_7_16_X1, D_7_16_X1, D_7_16_X1},
    {-D_7_16_X1, -D_7_16_X2, D_7_16_X2, D_7_16_X1, -D_7_16_X1, -D_7_16_X2, D_7_16_X2, D_7_16_X1,
     -D_7_16_X1, -D_7_16_X2, D_7_16_X2, D_7_16_X1, -D_7_16_X1, -D_7_16_X2, D_7_16_X2, D_7_16_X1},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {D_7_16_W1 * D_7_16_W1, D_7_16_W1 * D_7_16_W2, D_7_16_W1 * D_7_16_W2, D_7_16_W1 * D_7_16_W1,
     D_7_16_W2 * D_7_16_W1, D_7_16_W2 * D_7_16_W2, D_7_16_W2 * D_7_16_W2, D_7_16_W2 * D_7_16_W1,
     D_7_16_W2 * D_7_16_W1, D_7_16_W2 * D_7_16_W2, D_7_16_W2 * D_7_16_W2, D_7_16_W2 * D_7_16_W1,
     D_7_16_W1 * D_7_16_W1, D_7_16_W1 * D_7_16_W2, D_7_16_W1 * D_7_16_W2, D_7_16_W1 * D_7_16_W1}
};

/**
Degree 8, 16 positions from Wissman & Becker (cfr supra)

We chose formula 8-2 on p 684 since it seems to have nicest weights and
abscissa.

The minimal number of nodes is 15, but the one known rule that achieves this
minial number of nodes has nodes outside the unit square. That's not a
desirable situation for us (see Cools & Rabinowitz ...).
Btw, the formula of degree 9 has only one point more than this one.
*/
CubatureRule *
QuadCubatureRule::degree8QuadrilateralRule() {
    static CubatureRule degree8QuadrilateralRule = {
        "quads degree 8, 16 positions",
        16,
        {0.0, 0.0, 0.952509466071562, -0.952509466071562,
         0.532327454074206, -0.532327454074206, 0.684736297951735, -0.684736297951735,
         0.233143240801405, -0.233143240801405, 0.927683319306117, -0.927683319306117,
         0.453120687403749, -0.453120687403749, 0.837503640422812, -0.837503640422812},
        {0.659560131960342, -0.949142923043125, 0.765051819557684, 0.765051819557684,
         0.936975981088416, 0.936975981088416, 0.333656717735747, 0.333656717735747,
         -0.079583272377397, -0.079583272377397, -0.272240080612534, -0.272240080612534,
         -0.613735353398028, -0.613735353398028, -0.888477650535971, -0.888477650535971},
        {0.0, 0.0, 0.0, 0.0,
         0.0, 0.0, 0.0, 0.0,
         0.0, 0.0, 0.0, 0.0,
         0.0, 0.0, 0.0, 0.0},
        {0.450276776305590, 0.166570426777813, 0.098869459933431, 0.098869459933431,
         0.153696747140812, 0.153696747140812, 0.396686976072903, 0.396686976072903,
         0.352014367945695, 0.352014367945695, 0.189589054577798, 0.189589054577798,
         0.375101001147587, 0.375101001147587, 0.125618791640072, 0.125618791640072}
    };
    static bool transformed = false;
    if ( !transformed ) {
        transformQuadRuleInPlace(&degree8QuadrilateralRule);
        transformed = true;
    }

    return &degree8QuadrilateralRule;
}

/**
Degree 9, 17 positions, Moeller, "Kubaturformeln mit minimaler Knotenzahl, Numer. Math. 25, 185 (1976)
*/
CubatureRule QuadCubatureRule::crq9 = {
    "quads degree 9, 17 positions",
    17,
    {0.0,
     D_9_17_B1, -D_9_17_B1, -D_9_17_C1, D_9_17_C1,
     D_9_17_B2, -D_9_17_B2, -D_9_17_C2, D_9_17_C2,
     D_9_17_B3, -D_9_17_B3, -D_9_17_C3, D_9_17_C3,
     D_9_17_B4, -D_9_17_B4, -D_9_17_C4, D_9_17_C4},
    {0.0,
     D_9_17_C1, -D_9_17_C1, D_9_17_B1, -D_9_17_B1,
     D_9_17_C2, -D_9_17_C2, D_9_17_B2, -D_9_17_B2,
     D_9_17_C3, -D_9_17_C3, D_9_17_B3, -D_9_17_B3,
     D_9_17_C4, -D_9_17_C4, D_9_17_B4, -D_9_17_B4},
    {0.0,
     0.0, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0},
    {D_9_17_W0,
     D_9_17_W1, D_9_17_W1, D_9_17_W1, D_9_17_W1,
     D_9_17_W2, D_9_17_W2, D_9_17_W2, D_9_17_W2,
     D_9_17_W3, D_9_17_W3, D_9_17_W3, D_9_17_W3,
     D_9_17_W4, D_9_17_W4, D_9_17_W4, D_9_17_W4}
};

/**
Boxes: [-1, 1] ^ 3
*/

// Degree 1, 9 positions
CubatureRule *
QuadCubatureRule::degree1BoxRule() {
    static CubatureRule degree1BoxRule = {
        "boxes degree 1, 9 positions (the corners + center)",
        9,
        {D_1_9_U, D_1_9_U, D_1_9_U, D_1_9_U, -D_1_9_U, -D_1_9_U, -D_1_9_U, -D_1_9_U, 0.0},
        {D_1_9_U, D_1_9_U, -D_1_9_U, -D_1_9_U, D_1_9_U, D_1_9_U, -D_1_9_U, -D_1_9_U, 0.0},
        {D_1_9_U, -D_1_9_U, D_1_9_U, -D_1_9_U, D_1_9_U, -D_1_9_U, D_1_9_U, -D_1_9_U, 0.0},
        {D_1_9_W, D_1_9_W, D_1_9_W, D_1_9_W, D_1_9_W, D_1_9_W, D_1_9_W, D_1_9_W, D_1_9_W}
    };
    static bool transformed = false;
    if ( !transformed ) {
        transformCubeRuleInPlace(&degree1BoxRule);
        transformed = true;
    }

    return &degree1BoxRule;
}

// Degree 3, 8 positions, product Gauss-Legendre formula
CubatureRule QuadCubatureRule::crv3Pg = {
    "boxes degree 3, 8 positions, product Gauss formula",
    8,
    {D_3_8_U, D_3_8_U, D_3_8_U, D_3_8_U, -D_3_8_U, -D_3_8_U, -D_3_8_U, -D_3_8_U},
    {D_3_8_U, D_3_8_U, -D_3_8_U, -D_3_8_U, D_3_8_U, D_3_8_U, -D_3_8_U, -D_3_8_U},
    {D_3_8_U, -D_3_8_U, D_3_8_U, -D_3_8_U, D_3_8_U, -D_3_8_U, D_3_8_U, -D_3_8_U},
    {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0}
};

// quadprodrule[i-1] is a product rule of degree 2i+1 over [-1,1]^2
CubatureRule *const QuadCubatureRule::quadProductRule[3] = {
    &QuadCubatureRule::crq3Pg,
    &QuadCubatureRule::crq5Pg,
    &QuadCubatureRule::crq7Pg
};

// boxesprodrule[i-1] is a product rule of degree 2i+1 over [-1,1]^3
CubatureRule *const QuadCubatureRule::boxesProductRule[1] = {&QuadCubatureRule::crv3Pg};

/**
Installs cubature rules for triangles and quadrilaterals of the specified degree
*/
void
QuadCubatureRule::setQuadCubatureRules(CubatureRule **quadRule, const CubatureDegree degree) {
    ensureRulesTransformed();

    switch ( degree ) {
        case DEGREE_1:
            *quadRule = &crq1;
            break;
        case DEGREE_2:
            *quadRule = &crq2;
            break;
        case DEGREE_3:
            *quadRule = &crq3;
            break;
        case DEGREE_4:
            *quadRule = &crq4;
            break;
        case DEGREE_5:
            *quadRule = &crq5;
            break;
        case DEGREE_6:
            *quadRule = &crq6;
            break;
        case DEGREE_7:
            *quadRule = &crq7;
            break;
        case DEGREE_8:
            *quadRule = QuadCubatureRule::degree8QuadrilateralRule();
            break;
        case DEGREE_9:
            *quadRule = &crq9;
            break;
        case DEGREE_3_PROD:
            *quadRule = &crq3Pg;
            break;
        case DEGREE_5_PROD:
            *quadRule = &crq5Pg;
            break;
        case DEGREE_7_PROD:
            *quadRule = &crq7Pg;
            break;
        default:
            Logger::fatal(2, "setQuadCubatureRules", "Invalid degree %d", degree);
    }
}
