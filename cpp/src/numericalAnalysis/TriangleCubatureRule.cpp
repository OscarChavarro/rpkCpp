/**
Note: a "cubature rule" is a numerical integration method used to approximate
integration of functions in several dimensions.

Good cubature rules of degree <= 9 for quadrilaterals and triangles
Philippe Bekaert - Department of Computer Science, K. U. Leuven (Belgium)
Philippe.Bekaert@cs.kuleuven.ac.be
September, 5 1995
*/

#include "common/Error.h"
#include "numericalAnalysis/TriangleCubatureRule.h"

/**
Triangles: barycentric coordinates
Weights sum to 1. instead of 0.5, which is the area of the triangle
0 <= x+y <= 1, x,y >= 0
*/

// Degree 1, 1 point
CubatureRule TriangleCubatureRule::crt1 = {
    "triangles degree 1, 1 positions",
    1,
    {1.0 / 3.0},
    {1.0 / 3.0},
    {0.0},
    {1.0}
};

// Degree 2, 3 positions, Stroud '71 p 307
CubatureRule TriangleCubatureRule::crt2 = {
    "triangles degree 2, 3 positions",
    3,
    {1.0 / 6.0, 1.0 / 6.0, 2.0 / 3.0},
    {1.0 / 6.0, 2.0 / 3.0, 1.0 / 6.0},
    {0.0, 0.0, 0.0},
    {1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0}
};

// Degree 3, 4 positions, Stroud '71 p 308
CubatureRule TriangleCubatureRule::crt3 = {
    "triangles degree 3, 4 positions",
    4,
    {1.0 / 3.0, 0.2, 0.2, 0.6},
    {1.0 / 3.0, 0.2, 0.6, 0.2},
    {0.0, 0.0, 0.0, 0.0},
    {-9.0 / 16.0, 25.0 / 48.0, 25.0 / 48.0, 25.0 / 48.0}
};

/**
Degree 4, 6 positions
Lyness, Jespersen, "Moderate Degree Symmetric Quadrature Rules for the
Triangle", J. Inst. Maths. Applics (1975) 15, 19-32
*/
CubatureRule TriangleCubatureRule::crt4 = {
    "triangles degree 4, 6 positions",
    6,
    {TriangleCubatureRule::D_4_6_A1, TriangleCubatureRule::D_4_6_B1, TriangleCubatureRule::D_4_6_C1, TriangleCubatureRule::D_4_6_A2, TriangleCubatureRule::D_4_6_B2, TriangleCubatureRule::D_4_6_C2},
    {TriangleCubatureRule::D_4_6_B1, TriangleCubatureRule::D_4_6_C1, TriangleCubatureRule::D_4_6_A1, TriangleCubatureRule::D_4_6_B2, TriangleCubatureRule::D_4_6_C2, TriangleCubatureRule::D_4_6_A2},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {TriangleCubatureRule::D_4_6_W1, TriangleCubatureRule::D_4_6_W1, TriangleCubatureRule::D_4_6_W1, TriangleCubatureRule::D_4_6_W2, TriangleCubatureRule::D_4_6_W2, TriangleCubatureRule::D_4_6_W2}
};

// Degree 5, 7 positions, Stroud '71 p 314
CubatureRule TriangleCubatureRule::crt5 = {
    "triangles degree 5, 7 positions",
    7,
    {TriangleCubatureRule::D_5_7_T, TriangleCubatureRule::D_5_7_R, TriangleCubatureRule::D_5_7_R, TriangleCubatureRule::D_5_7_S, TriangleCubatureRule::D_5_7_U, TriangleCubatureRule::D_5_7_U, TriangleCubatureRule::D_5_7_V},
    {TriangleCubatureRule::D_5_7_T, TriangleCubatureRule::D_5_7_R, TriangleCubatureRule::D_5_7_S, TriangleCubatureRule::D_5_7_R, TriangleCubatureRule::D_5_7_U, TriangleCubatureRule::D_5_7_V, TriangleCubatureRule::D_5_7_U},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {TriangleCubatureRule::D_5_7_A, TriangleCubatureRule::D_5_7_B, TriangleCubatureRule::D_5_7_B, TriangleCubatureRule::D_5_7_B, TriangleCubatureRule::D_5_7_C, TriangleCubatureRule::D_5_7_C, TriangleCubatureRule::D_5_7_C}
};

/**
Degree 7, 12 nodes, Gaterman, "The Construction of Symmetric Cubature Formulas for the
Square and the triangle", Computing, 40, 229-240 (1988)
*/
CubatureRule TriangleCubatureRule::crt7 = {
    "triangles degree 7, 12 positions",
    12,
    {TriangleCubatureRule::D_7_12_A1, TriangleCubatureRule::D_7_12_B1, TriangleCubatureRule::D_7_12_C1, TriangleCubatureRule::D_7_12_A2, TriangleCubatureRule::D_7_12_B2, TriangleCubatureRule::D_7_12_C2,
     TriangleCubatureRule::D_7_12_A3, TriangleCubatureRule::D_7_12_B3, TriangleCubatureRule::D_7_12_C3, TriangleCubatureRule::D_7_12_A4, TriangleCubatureRule::D_7_12_B4, TriangleCubatureRule::D_7_12_C4},
    {TriangleCubatureRule::D_7_12_B1, TriangleCubatureRule::D_7_12_C1, TriangleCubatureRule::D_7_12_A1, TriangleCubatureRule::D_7_12_B2, TriangleCubatureRule::D_7_12_C2, TriangleCubatureRule::D_7_12_A2,
     TriangleCubatureRule::D_7_12_B3, TriangleCubatureRule::D_7_12_C3, TriangleCubatureRule::D_7_12_A3, TriangleCubatureRule::D_7_12_B4, TriangleCubatureRule::D_7_12_C4, TriangleCubatureRule::D_7_12_A4},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {TriangleCubatureRule::D_7_12_W1, TriangleCubatureRule::D_7_12_W1, TriangleCubatureRule::D_7_12_W1, TriangleCubatureRule::D_7_12_W2, TriangleCubatureRule::D_7_12_W2, TriangleCubatureRule::D_7_12_W2,
     TriangleCubatureRule::D_7_12_W3, TriangleCubatureRule::D_7_12_W3, TriangleCubatureRule::D_7_12_W3, TriangleCubatureRule::D_7_12_W4, TriangleCubatureRule::D_7_12_W4, TriangleCubatureRule::D_7_12_W4}
};

// Degree 8, 16 positions Lyness & Jespersen
CubatureRule *
TriangleCubatureRule::degree8Rule() {
    static CubatureRule degree8Rule = {
        "triangles degree 8, 16 positions",
        16,
        {TriangleCubatureRule::D_8_16_A0,
         TriangleCubatureRule::D_8_16_A1, TriangleCubatureRule::D_8_16_B1, TriangleCubatureRule::D_8_16_C1,
         TriangleCubatureRule::D_8_16_A2, TriangleCubatureRule::D_8_16_B2, TriangleCubatureRule::D_8_16_C2,
         TriangleCubatureRule::D_8_16_A3, TriangleCubatureRule::D_8_16_B3, TriangleCubatureRule::D_8_16_C3,
         TriangleCubatureRule::D_8_16_A4, TriangleCubatureRule::D_8_16_A4, TriangleCubatureRule::D_8_16_B4, TriangleCubatureRule::D_8_16_B4, TriangleCubatureRule::D_8_16_C4, TriangleCubatureRule::D_8_16_C4},
        {TriangleCubatureRule::D_8_16_B0,
         TriangleCubatureRule::D_8_16_B1, TriangleCubatureRule::D_8_16_C1, TriangleCubatureRule::D_8_16_A1,
         TriangleCubatureRule::D_8_16_B2, TriangleCubatureRule::D_8_16_C2, TriangleCubatureRule::D_8_16_A2,
         TriangleCubatureRule::D_8_16_B3, TriangleCubatureRule::D_8_16_C3, TriangleCubatureRule::D_8_16_A3,
         TriangleCubatureRule::D_8_16_B4, TriangleCubatureRule::D_8_16_C4, TriangleCubatureRule::D_8_16_C4, TriangleCubatureRule::D_8_16_A4, TriangleCubatureRule::D_8_16_A4, TriangleCubatureRule::D_8_16_B4},
        {0.0,
         0.0, 0.0, 0.0,
         0.0, 0.0, 0.0,
         0.0, 0.0, 0.0,
         0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
        {TriangleCubatureRule::D_8_16_W0,
         TriangleCubatureRule::D_8_16_W1, TriangleCubatureRule::D_8_16_W1, TriangleCubatureRule::D_8_16_W1,
         TriangleCubatureRule::D_8_16_W2, TriangleCubatureRule::D_8_16_W2, TriangleCubatureRule::D_8_16_W2,
         TriangleCubatureRule::D_8_16_W3, TriangleCubatureRule::D_8_16_W3, TriangleCubatureRule::D_8_16_W3,
         TriangleCubatureRule::D_8_16_W4, TriangleCubatureRule::D_8_16_W4, TriangleCubatureRule::D_8_16_W4, TriangleCubatureRule::D_8_16_W4, TriangleCubatureRule::D_8_16_W4, TriangleCubatureRule::D_8_16_W4}
    };

    return &degree8Rule;
}

/**
Degree 9, 19 positions - there is as yet no rule over the triangle known
which has only the minimal number of 17 nodes (see Cools & Rabinowitz).
Lyness & Jespersen
*/
CubatureRule TriangleCubatureRule::crt9 = {
    "triangles degree 9, 19 positions",
    19,
    {TriangleCubatureRule::D_9_19_A0,
     TriangleCubatureRule::D_9_19_A1, TriangleCubatureRule::D_9_19_B1, TriangleCubatureRule::D_9_19_C1,
     TriangleCubatureRule::D_9_19_A2, TriangleCubatureRule::D_9_19_B2, TriangleCubatureRule::D_9_19_C2,
     TriangleCubatureRule::D_9_19_A3, TriangleCubatureRule::D_9_19_B3, TriangleCubatureRule::D_9_19_C3,
     TriangleCubatureRule::D_9_19_A4, TriangleCubatureRule::D_9_19_B4, TriangleCubatureRule::D_9_19_C4,
     TriangleCubatureRule::D_9_19_A5, TriangleCubatureRule::D_9_19_A5, TriangleCubatureRule::D_9_19_B5,
     TriangleCubatureRule::D_9_19_B5, TriangleCubatureRule::D_9_19_C5, TriangleCubatureRule::D_9_19_C5},
    {TriangleCubatureRule::D_9_19_B0,
     TriangleCubatureRule::D_9_19_B1, TriangleCubatureRule::D_9_19_C1, TriangleCubatureRule::D_9_19_A1,
     TriangleCubatureRule::D_9_19_B2, TriangleCubatureRule::D_9_19_C2, TriangleCubatureRule::D_9_19_A2,
     TriangleCubatureRule::D_9_19_B3, TriangleCubatureRule::D_9_19_C3, TriangleCubatureRule::D_9_19_A3,
     TriangleCubatureRule::D_9_19_B4, TriangleCubatureRule::D_9_19_C4, TriangleCubatureRule::D_9_19_A4,
     TriangleCubatureRule::D_9_19_B5, TriangleCubatureRule::D_9_19_C5, TriangleCubatureRule::D_9_19_C5,
     TriangleCubatureRule::D_9_19_A5, TriangleCubatureRule::D_9_19_A5, TriangleCubatureRule::D_9_19_B5},
    {0.0,
     0.0, 0.0, 0.0,
     0.0, 0.0, 0.0,
     0.0, 0.0, 0.0,
     0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {TriangleCubatureRule::D_9_19_W0,
     TriangleCubatureRule::D_9_19_W1, TriangleCubatureRule::D_9_19_W1, TriangleCubatureRule::D_9_19_W1,
     TriangleCubatureRule::D_9_19_W2, TriangleCubatureRule::D_9_19_W2, TriangleCubatureRule::D_9_19_W2,
     TriangleCubatureRule::D_9_19_W3, TriangleCubatureRule::D_9_19_W3, TriangleCubatureRule::D_9_19_W3,
     TriangleCubatureRule::D_9_19_W4, TriangleCubatureRule::D_9_19_W4, TriangleCubatureRule::D_9_19_W4,
     TriangleCubatureRule::D_9_19_W5, TriangleCubatureRule::D_9_19_W5, TriangleCubatureRule::D_9_19_W5,
     TriangleCubatureRule::D_9_19_W5, TriangleCubatureRule::D_9_19_W5, TriangleCubatureRule::D_9_19_W5}
};

/**
Installs cubature rules for triangles and quadrilaterals of the specified degree
*/
void
TriangleCubatureRule::setTriangleCubatureRules(CubatureRule **triRule, const CubatureDegree degree) {
    switch ( degree ) {
        case CubatureDegree::DEGREE_1:
            *triRule = &crt1;
            break;
        case CubatureDegree::DEGREE_2:
            *triRule = &crt2;
            break;
        case CubatureDegree::DEGREE_3:
            *triRule = &crt3;
            break;
        case CubatureDegree::DEGREE_4:
            *triRule = &crt4;
            break;
        case CubatureDegree::DEGREE_5:
            *triRule = &crt5;
            break;
        case CubatureDegree::DEGREE_6:
        case CubatureDegree::DEGREE_7:
            *triRule = &crt7;
            break;
        case CubatureDegree::DEGREE_8:
            *triRule = TriangleCubatureRule::degree8Rule();
            break;
        case CubatureDegree::DEGREE_9:
            *triRule = &crt9;
            break;
        case CubatureDegree::DEGREE_3_PROD:
            *triRule = &crt5;
            break;
        case CubatureDegree::DEGREE_5_PROD:
            *triRule = &crt7;
            break;
        case CubatureDegree::DEGREE_7_PROD:
            *triRule = &crt9;
            break;
        default:
            Error::fatal(2, "setTriangleCubatureRules", "Invalid degree %d", degree);
    }
}
