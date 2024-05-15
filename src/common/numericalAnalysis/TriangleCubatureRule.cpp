/**
Note: a "cubature rule" is a numerical integration method used to approximate
integration of functions in several dimensions.

Good cubature rules of degree <= 9 for quadrilaterals and triangles
Philippe Bekaert - Department of Computer Science, K. U. Leuven (Belgium)
Philippe.Bekaert@cs.kuleuven.ac.be
September, 5 1995
*/

#include "common/error.h"
#include "common/numericalAnalysis/TriangleCubatureRule.h"

/**
Triangles: barycentric coordinates
Weights sum to 1. instead of 0.5, which is the area of the triangle
0 <= x+y <= 1, x,y >= 0
*/

// Degree 1, 1 point
static CubatureRule globalCrt1 = {
    "triangles degree 1, 1 positions",
    1,
    {1.0 / 3.0},
    {1.0 / 3.0},
    {0.0},
    {1.0}
};

// Degree 2, 3 positions, Stroud '71 p 307
static CubatureRule globalCrt2 = {
    "triangles degree 2, 3 positions",
    3,
    {1.0 / 6.0, 1.0 / 6.0, 2.0 / 3.0},
    {1.0 / 6.0, 2.0 / 3.0, 1.0 / 6.0},
    {0.0, 0.0, 0.0},
    {1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0}
};

// Degree 3, 4 positions, Stroud '71 p 308
static CubatureRule globalCrt3 = {
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
static const double D_4_6_W1 = 3.298552309659655e-1 / 3.0;
static const double D_4_6_A1 = 8.168475729804585e-1;
static const double D_4_6_B1 = 9.157621350977073e-2;
static const double D_4_6_C1 = D_4_6_B1;
static const double D_4_6_W2 = 6.701447690340345e-1 / 3.0;
static const double D_4_6_A2 = 1.081030181680702e-1;
static const double D_4_6_B2 = 4.459484909159649e-1;
static const double D_4_6_C2 = D_4_6_B2;
static CubatureRule globalCrt4 = {
    "triangles degree 4, 6 positions",
    6,
    {D_4_6_A1, D_4_6_B1, D_4_6_C1, D_4_6_A2, D_4_6_B2, D_4_6_C2},
    {D_4_6_B1, D_4_6_C1, D_4_6_A1, D_4_6_B2, D_4_6_C2, D_4_6_A2},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {D_4_6_W1, D_4_6_W1, D_4_6_W1, D_4_6_W2, D_4_6_W2, D_4_6_W2}
};

// Degree 5, 7 positions, Stroud '71 p 314
static const double D_5_7_R = 0.1012865073234563;
static const double D_5_7_S = 0.7974269853530873;
static const double D_5_7_T = 1.0 / 3.0;
static const double D_5_7_U = 0.4701420641051151;
static const double D_5_7_V = 0.05971587178976981;
static const double D_5_7_A = 0.225;
static const double D_5_7_B = 0.1259391805448271;
static const double D_5_7_C = 0.1323941527885062;
static CubatureRule globalCrt5 = {
    "triangles degree 5, 7 positions",
    7,
    {D_5_7_T, D_5_7_R, D_5_7_R, D_5_7_S, D_5_7_U, D_5_7_U, D_5_7_V},
    {D_5_7_T, D_5_7_R, D_5_7_S, D_5_7_R, D_5_7_U, D_5_7_V, D_5_7_U},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {D_5_7_A, D_5_7_B, D_5_7_B, D_5_7_B, D_5_7_C, D_5_7_C, D_5_7_C}
};

/**
Degree 7, 12 nodes, Gaterman, "The Construction of Symmetric Cubature Formulas for the
Square and the triangle", Computing, 40, 229-240 (1988)
*/
static const double D_7_12_W1 = 0.2651702815743450e-1 * 2.0;
static const double D_7_12_A1 = 0.6238226509439084e-1;
static const double D_7_12_B1 = 0.6751786707392436e-1;
static const double D_7_12_C1 = 0.8700998678316848;
static const double D_7_12_W2 = 0.4388140871444811e-1 * 2.0;
static const double D_7_12_A2 = 0.5522545665692000e-1;
static const double D_7_12_B2 = 0.3215024938520156;
static const double D_7_12_C2 = 0.6232720494910644;
static const double D_7_12_W3 = 0.2877504278497528e-1 * 2.0;
static const double D_7_12_A3 = 0.3432430294509488e-1;
static const double D_7_12_B3 = 0.6609491961867980;
static const double D_7_12_C3 = 0.3047265008681072;
static const double D_7_12_W4 = 0.6749318700980879e-1 * 2.0;
static const double D_7_12_A4 = 0.5158423343536001;
static const double D_7_12_B4 = 0.2777161669764050;
static const double D_7_12_C4 = 0.2064414986699949;
static CubatureRule globalCrt7 = {
    "triangles degree 7, 12 positions",
    12,
    {D_7_12_A1, D_7_12_B1, D_7_12_C1, D_7_12_A2, D_7_12_B2, D_7_12_C2,
     D_7_12_A3, D_7_12_B3, D_7_12_C3, D_7_12_A4, D_7_12_B4, D_7_12_C4},
    {D_7_12_B1, D_7_12_C1, D_7_12_A1, D_7_12_B2, D_7_12_C2, D_7_12_A2,
     D_7_12_B3, D_7_12_C3, D_7_12_A3, D_7_12_B4, D_7_12_C4, D_7_12_A4},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {D_7_12_W1, D_7_12_W1, D_7_12_W1, D_7_12_W2, D_7_12_W2, D_7_12_W2,
     D_7_12_W3, D_7_12_W3, D_7_12_W3, D_7_12_W4, D_7_12_W4, D_7_12_W4}
};

// Degree 8, 16 positions Lyness & Jespersen
static const double D_8_16_W0 = 1.443156076777862e-1;
static const double D_8_16_A0 = 3.333333333333333e-1;
static const double D_8_16_B0 = 3.333333333333333e-1;
static const double D_8_16_W1 = 2.852749028018549e-1 / 3.0;
static const double D_8_16_A1 = 8.141482341455413e-2;
static const double D_8_16_B1 = 4.592925882927229e-1;
static const double D_8_16_C1 = D_8_16_B1;
static const double D_8_16_W2 = 9.737549286959440e-2 / 3.0;
static const double D_8_16_A2 = 8.989055433659379e-1;
static const double D_8_16_B2 = 5.054722831703103e-2;
static const double D_8_16_C2 = D_8_16_B2;
static const double D_8_16_W3 = 3.096521116041552e-1 / 3.0;
static const double D_8_16_A3 = 6.588613844964797e-1;
static const double D_8_16_B3 = 1.705693077517601e-1;
static const double D_8_16_C3 = D_8_16_B3;
static const double D_8_16_W4 = 1.633818850466092e-1 / 6.0;
static const double D_8_16_A4 = 8.394777409957211e-3;
static const double D_8_16_B4 = 7.284923929554041e-1;
static const double D_8_16_C4 = 2.631128296346387e-1;
CubatureRule GLOBAL_crt8 = {
    "triangles degree 8, 16 positions",
    16,
    {D_8_16_A0,
     D_8_16_A1, D_8_16_B1, D_8_16_C1,
     D_8_16_A2, D_8_16_B2, D_8_16_C2,
     D_8_16_A3, D_8_16_B3, D_8_16_C3,
     D_8_16_A4, D_8_16_A4, D_8_16_B4, D_8_16_B4, D_8_16_C4, D_8_16_C4},
    {D_8_16_B0,
     D_8_16_B1, D_8_16_C1, D_8_16_A1,
     D_8_16_B2, D_8_16_C2, D_8_16_A2,
     D_8_16_B3, D_8_16_C3, D_8_16_A3,
     D_8_16_B4, D_8_16_C4, D_8_16_C4, D_8_16_A4, D_8_16_A4, D_8_16_B4},
    {0.0,
     0.0, 0.0, 0.0,
     0.0, 0.0, 0.0,
     0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {D_8_16_W0,
     D_8_16_W1, D_8_16_W1, D_8_16_W1,
     D_8_16_W2, D_8_16_W2, D_8_16_W2,
     D_8_16_W3, D_8_16_W3, D_8_16_W3,
     D_8_16_W4, D_8_16_W4, D_8_16_W4, D_8_16_W4, D_8_16_W4, D_8_16_W4}
};

/**
Degree 9, 19 positions - there is as yet no rule over the triangle known
which has only the minimal number of 17 nodes (see Cools & Rabinowitz).
Lyness & Jespersen
*/
static const double D_9_19_W0 = 9.713579628279610e-2;
static const double D_9_19_A0 = 3.333333333333333e-1;
static const double D_9_19_B0 = 3.333333333333333e-1;
static const double D_9_19_W1 = 9.400410068141950e-2 / 3.0;
static const double D_9_19_A1 = 2.063496160252593e-2;
static const double D_9_19_B1 = 4.896825191987370e-1;
static const double D_9_19_C1 = D_9_19_B1;
static const double D_9_19_W2 = 2.334826230143263e-1 / 3.0;
static const double D_9_19_A2 = 1.258208170141290e-1;
static const double D_9_19_B2 = 4.370895914929355e-1;
static const double D_9_19_C2 = D_9_19_B2;
static const double D_9_19_W3 = 2.389432167816273e-1 / 3.0;
static const double D_9_19_A3 = 6.235929287619356e-1;
static const double D_9_19_B3 = 1.882035356190322e-1;
static const double D_9_19_C3 = D_9_19_B3;
static const double D_9_19_W4 = 7.673302697609430e-2 / 3.0;
static const double D_9_19_A4 = 9.105409732110941e-1;
static const double D_9_19_B4 = 4.472951339445297e-2;
static const double D_9_19_C4 = D_9_19_B4;
static const double D_9_19_W5 = 2.597012362637364e-1 / 6.0;
static const double D_9_19_A5 = 3.683841205473626e-2;
static const double D_9_19_B5 = 7.411985987844980e-1;
static const double D_9_19_C5 = 2.219629891607657e-1;
static CubatureRule globalCrt9 = {
    "triangles degree 9, 19 positions",
    19,
    {D_9_19_A0,
     D_9_19_A1, D_9_19_B1, D_9_19_C1,
     D_9_19_A2, D_9_19_B2, D_9_19_C2,
     D_9_19_A3, D_9_19_B3, D_9_19_C3,
     D_9_19_A4, D_9_19_B4, D_9_19_C4,
     D_9_19_A5, D_9_19_A5, D_9_19_B5,
     D_9_19_B5, D_9_19_C5, D_9_19_C5},
    {D_9_19_B0,
     D_9_19_B1, D_9_19_C1, D_9_19_A1,
     D_9_19_B2, D_9_19_C2, D_9_19_A2,
     D_9_19_B3, D_9_19_C3, D_9_19_A3,
     D_9_19_B4, D_9_19_C4, D_9_19_A4,
     D_9_19_B5, D_9_19_C5, D_9_19_C5,
     D_9_19_A5, D_9_19_A5, D_9_19_B5},
    {0.0,
     0.0, 0.0, 0.0,
     0.0, 0.0, 0.0,
     0.0, 0.0, 0.0,
     0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {D_9_19_W0,
     D_9_19_W1, D_9_19_W1, D_9_19_W1,
     D_9_19_W2, D_9_19_W2, D_9_19_W2,
     D_9_19_W3, D_9_19_W3, D_9_19_W3,
     D_9_19_W4, D_9_19_W4, D_9_19_W4,
     D_9_19_W5, D_9_19_W5, D_9_19_W5,
     D_9_19_W5, D_9_19_W5, D_9_19_W5}
};

/**
Installs cubature rules for triangles and quadrilaterals of the specified degree
*/
void
TriangleCubatureRule::setTriangleCubatureRules(CubatureRule **triRule, const CubatureDegree degree) {
    switch ( degree ) {
        case CubatureDegree::DEGREE_1:
            *triRule = &globalCrt1;
            break;
        case CubatureDegree::DEGREE_2:
            *triRule = &globalCrt2;
            break;
        case CubatureDegree::DEGREE_3:
            *triRule = &globalCrt3;
            break;
        case CubatureDegree::DEGREE_4:
            *triRule = &globalCrt4;
            break;
        case CubatureDegree::DEGREE_5:
            *triRule = &globalCrt5;
            break;
        case CubatureDegree::DEGREE_6:
        case CubatureDegree::DEGREE_7:
            *triRule = &globalCrt7;
            break;
        case CubatureDegree::DEGREE_8:
            *triRule = &GLOBAL_crt8;
            break;
        case CubatureDegree::DEGREE_9:
            *triRule = &globalCrt9;
            break;
        case CubatureDegree::DEGREE_3_PROD:
            *triRule = &globalCrt5;
            break;
        case CubatureDegree::DEGREE_5_PROD:
            *triRule = &globalCrt7;
            break;
        case CubatureDegree::DEGREE_7_PROD:
            *triRule = &globalCrt9;
            break;
        default:
            logFatal(2, "setTriangleCubatureRules", "Invalid degree %d", degree);
    }
}
