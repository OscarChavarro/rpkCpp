package vsdk.toolkit.numericalAnalysis;

import vsdk.toolkit.common.Error;
/**
quadrilateral rules are specified below in the canonical [-1, 1]^2 domain and
pretransformed during static initialization to [0, 1]^2.
*/

/**
Degree 1, 1 point
*/

/**
Degree 2, 3 positions Stroud '71
*/

/**
Degree 3, 4 positions, Davis & Rabinowitz, Methods of Numerical Integration,
2nd edition 1984, p 367
*/

// Degree 3, 4 positions, product Gauss-Legendre formula

// sqrt(1/3)

// 1st coord. of abscissa

// 2nd coord. of abscissa

// 3rd coord. of abscissa

// Weights

/**
Degree 4, 6 positions
see: Wissman, Becker, "Partially Symmetric Cubature Formulas for Even
Degrees of Exactness", SIAM. J. Numer. Anal., Vol 23 nr 3 (1986), p 676
You'll find also another similar rule in this paper, but I chose this one
because the abscissa seem to be nicer located.
You'll find the same rule in: Schmid, "On Cubature Formulae with a Minimal
Number of Knots", Numer. Math. Vol 31 (1978) p281
*/

// Degree 5, 7 positions, Radon's rule see e.g. Stroud '71

// Degree 5, 9 positions product Gauss-Legendre rule

// abscissa and weights computed using Stuff/gauleg.c

/**
Degree 6, 10 positions
from: Wissmann & Becker (cfr supra)
They again give two formulae of this type and you'll also find one
in Schmid, but I chose this one because it has the nicest weights
*/

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

// Degree 7, 16 positions product Gauss rule

/**
Degree 8, 16 positions from Wissman & Becker (cfr supra)

We chose formula 8-2 on p 684 since it seems to have nicest weights and
abscissa.

The minimal number of nodes is 15, but the one known rule that achieves this
minial number of nodes has nodes outside the unit square. That's not a
desirable situation for us (see Cools & Rabinowitz ...).
Btw, the formula of degree 9 has only one point more than this one.
*/

/**
Degree 9, 17 positions, Moeller, "Kubaturformeln mit minimaler Knotenzahl, Numer. Math. 25, 185 (1976)
*/

/**
Boxes: [-1, 1] ^ 3
*/

// Degree 3, 8 positions, product Gauss-Legendre formula

/**
Installs cubature rules for triangles and quadrilaterals of the specified degree
*/


public class QuadCubatureRule {
    private static final double D_2_3_W = 4.0 / 3.0;
    private static final double D_2_3_U = 0.81649658092772603272;
    private static final double D_2_3_C = -0.5;
    private static final double D_2_3_S = 0.86602540378443864676;

    private static final double D_3_4_U = 0.81649658092772603272;
    private static final double D_3_4_G_U = 0.57735026918962576450;

    private static final double D_5_7_W1 = 8.0 / 7.0;
    private static final double D_5_7_W2 = 5.0 / 9.0;
    private static final double D_5_7_W3 = 20.0 / 63.0;
    private static final double D_5_7_R = 0.96609178307929588492;
    private static final double D_5_7_S = 0.57735026918962573106;
    private static final double D_5_7_T = 0.77459666924148340428;

    private static final double D_5_9_X0 = 0.0;
    private static final double D_5_9_W0 = 8.0 / 9.0;
    private static final double D_5_9_X1 = 0.7745966692414834;
    private static final double D_5_9_W1 = 5.0 / 9.0;

    private static final double D_7_12_R = 0.92582009977255141919;
    private static final double D_7_12_S = 0.38055443320831561227;
    private static final double D_7_12_T = 0.80597978291859884159;
    private static final double D_7_12_W1 = 0.24197530864197530631;
    private static final double D_7_12_W2 = 0.52059291666739448967;
    private static final double D_7_12_W3 = 0.23743177469063023177;

    private static final double D_7_16_X1 = 0.86113631159405257522;
    private static final double D_7_16_X2 = 0.33998104358485626480;
    private static final double D_7_16_W1 = 0.34785484513745385737;
    private static final double D_7_16_W2 = 0.65214515486254614263;

    private static final double D_9_17_B1 = 0.96884996636197772072;
    private static final double D_9_17_B2 = 0.75027709997890053354;
    private static final double D_9_17_B3 = 0.52373582021442933604;
    private static final double D_9_17_B4 = 0.07620832819261717318;
    private static final double D_9_17_C1 = 0.63068011973166885417;
    private static final double D_9_17_C2 = 0.92796164595956966740;
    private static final double D_9_17_C3 = 0.45333982113564719076;
    private static final double D_9_17_C4 = 0.85261572933366230775;
    private static final double D_9_17_W0 = 0.52674897119341563786;
    private static final double D_9_17_W1 = 0.08887937817019870697;
    private static final double D_9_17_W2 = 0.11209960212959648528;
    private static final double D_9_17_W3 = 0.39828243926207009528;
    private static final double D_9_17_W4 = 0.26905133763978080301;

    private static final double D_1_9_U = 1.0;
    private static final double D_1_9_W = 8.0 / 9.0;
    private static final double D_3_8_U = 0.57735026918962576450;

    private static CubatureRule crq1 = createTransformedQuadRule(new CubatureRule(
        "quads degree 1, 1 point",
        1,
        new double[] {0.0},
        new double[] {0.0},
        new double[] {0.0},
        new double[] {4.0}));

    private static CubatureRule crq2 = createTransformedQuadRule(new CubatureRule(
        "quads degree 2, 3 positions",
        3,
        new double[] {D_2_3_U, D_2_3_U * D_2_3_C, D_2_3_U * D_2_3_C},
        new double[] {0.0, D_2_3_U * D_2_3_S, -D_2_3_U * D_2_3_S},
        new double[] {0.0, 0.0, 0.0},
        new double[] {D_2_3_W, D_2_3_W, D_2_3_W}));

    private static CubatureRule crq3 = createTransformedQuadRule(new CubatureRule(
        "quads degree 3, 4 positions",
        4,
        new double[] {D_3_4_U, 0.0, -D_3_4_U, 0.0},
        new double[] {0.0, D_3_4_U, 0.0, -D_3_4_U},
        new double[] {0.0, 0.0, 0.0, 0.0},
        new double[] {1.0, 1.0, 1.0, 1.0}));

    private static CubatureRule crq3Pg = createTransformedQuadRule(new CubatureRule(
        "quads degree 3, 4 positions, product Gauss formula",
        4,
        new double[] {D_3_4_G_U, D_3_4_G_U, -D_3_4_G_U, -D_3_4_G_U},
        new double[] {D_3_4_G_U, -D_3_4_G_U, D_3_4_G_U, -D_3_4_G_U},
        new double[] {0.0, 0.0, 0.0, 0.0},
        new double[] {1.0, 1.0, 1.0, 1.0}));

    private static CubatureRule crq4 = createTransformedQuadRule(new CubatureRule(
        "quads degree 4, 6 positions",
        6,
        new double[] {0.0, 0.0, 0.774596669241483, -0.774596669241483, 0.774596669241483, -0.774596669241483},
        new double[] {-0.356822089773090, 0.934172358962716, 0.390885162530071, 0.390885162530071, -0.852765377881771, -0.852765377881771},
        new double[] {0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
        new double[] {1.286412084888852, 0.491365692888926, 0.761883709085613, 0.761883709085613, 0.349227402025498, 0.349227402025498}));

    private static CubatureRule crq5 = createTransformedQuadRule(new CubatureRule(
        "quads degree 5, 7 positions, Radon's rule",
        7,
        new double[] {0.0, D_5_7_S, D_5_7_S, -D_5_7_S, -D_5_7_S, D_5_7_R, -D_5_7_R},
        new double[] {0.0, D_5_7_T, -D_5_7_T, D_5_7_T, -D_5_7_T, 0.0, 0.0},
        new double[] {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
        new double[] {D_5_7_W1, D_5_7_W2, D_5_7_W2, D_5_7_W2, D_5_7_W2, D_5_7_W3, D_5_7_W3}));

    private static CubatureRule crq5Pg = createTransformedQuadRule(new CubatureRule(
        "quads degree 5, 9 positions product Gauss rule",
        9,
        new double[] {-D_5_9_X1, -D_5_9_X1, -D_5_9_X1, D_5_9_X0, D_5_9_X0, D_5_9_X0, D_5_9_X1, D_5_9_X1, D_5_9_X1},
        new double[] {-D_5_9_X1, D_5_9_X0, D_5_9_X1, -D_5_9_X1, D_5_9_X0, D_5_9_X1, -D_5_9_X1, D_5_9_X0, D_5_9_X1},
        new double[] {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
        new double[] {
            D_5_9_W1 * D_5_9_W1, D_5_9_W1 * D_5_9_W0, D_5_9_W1 * D_5_9_W1,
            D_5_9_W0 * D_5_9_W1, D_5_9_W0 * D_5_9_W0, D_5_9_W0 * D_5_9_W1,
            D_5_9_W1 * D_5_9_W1, D_5_9_W1 * D_5_9_W0, D_5_9_W1 * D_5_9_W1}));

    private static CubatureRule crq6 = createTransformedQuadRule(new CubatureRule(
        "quads degree 6, 10 positions",
        10,
        new double[] {0.0, 0.0, 0.863742826346154, -0.863742826346154,
            0.518690521392592, -0.518690521392592, 0.933972544972849, -0.933972544972849,
            0.608977536016356, -0.608977536016356},
        new double[] {0.869833375250059, -0.479406351612111, 0.802837516207657, 0.802837516207657,
            0.262143665508058, 0.262143665508058, -0.363096583148066, -0.363096583148066,
            -0.896608632762453, -0.896608632762453},
        new double[] {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
        new double[] {0.392750590964348, 0.754762881242610, 0.206166050588279, 0.206166050588279,
            0.689992138489864, 0.689992138489864, 0.260517488732317, 0.260517488732317,
            0.269567586086061, 0.269567586086061}));

    private static CubatureRule crq7 = createTransformedQuadRule(new CubatureRule(
        "quads degree 7, 12 positions",
        12,
        new double[] {D_7_12_R, -D_7_12_R, 0.0, 0.0, D_7_12_S, D_7_12_S, -D_7_12_S, -D_7_12_S, D_7_12_T, D_7_12_T, -D_7_12_T, -D_7_12_T},
        new double[] {0.0, 0.0, D_7_12_R, -D_7_12_R, D_7_12_S, -D_7_12_S, D_7_12_S, -D_7_12_S, D_7_12_T, -D_7_12_T, D_7_12_T, -D_7_12_T},
        new double[] {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
        new double[] {D_7_12_W1, D_7_12_W1, D_7_12_W1, D_7_12_W1,
            D_7_12_W2, D_7_12_W2, D_7_12_W2, D_7_12_W2,
            D_7_12_W3, D_7_12_W3, D_7_12_W3, D_7_12_W3}));

    private static CubatureRule crq7Pg = createTransformedQuadRule(new CubatureRule(
        "quads degree 7, 16 positions product Gauss rule",
        16,
        new double[] {-D_7_16_X1, -D_7_16_X1, -D_7_16_X1, -D_7_16_X1, -D_7_16_X2, -D_7_16_X2, -D_7_16_X2, -D_7_16_X2,
            D_7_16_X2, D_7_16_X2, D_7_16_X2, D_7_16_X2, D_7_16_X1, D_7_16_X1, D_7_16_X1, D_7_16_X1},
        new double[] {-D_7_16_X1, -D_7_16_X2, D_7_16_X2, D_7_16_X1, -D_7_16_X1, -D_7_16_X2, D_7_16_X2, D_7_16_X1,
            -D_7_16_X1, -D_7_16_X2, D_7_16_X2, D_7_16_X1, -D_7_16_X1, -D_7_16_X2, D_7_16_X2, D_7_16_X1},
        new double[] {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
        new double[] {D_7_16_W1 * D_7_16_W1, D_7_16_W1 * D_7_16_W2, D_7_16_W1 * D_7_16_W2, D_7_16_W1 * D_7_16_W1,
            D_7_16_W2 * D_7_16_W1, D_7_16_W2 * D_7_16_W2, D_7_16_W2 * D_7_16_W2, D_7_16_W2 * D_7_16_W1,
            D_7_16_W2 * D_7_16_W1, D_7_16_W2 * D_7_16_W2, D_7_16_W2 * D_7_16_W2, D_7_16_W2 * D_7_16_W1,
            D_7_16_W1 * D_7_16_W1, D_7_16_W1 * D_7_16_W2, D_7_16_W1 * D_7_16_W2, D_7_16_W1 * D_7_16_W1}));

    private static CubatureRule crq9 = createTransformedQuadRule(new CubatureRule(
        "quads degree 9, 17 positions",
        17,
        new double[] {0.0,
            D_9_17_B1, -D_9_17_B1, -D_9_17_C1, D_9_17_C1,
            D_9_17_B2, -D_9_17_B2, -D_9_17_C2, D_9_17_C2,
            D_9_17_B3, -D_9_17_B3, -D_9_17_C3, D_9_17_C3,
            D_9_17_B4, -D_9_17_B4, -D_9_17_C4, D_9_17_C4},
        new double[] {0.0,
            D_9_17_C1, -D_9_17_C1, D_9_17_B1, -D_9_17_B1,
            D_9_17_C2, -D_9_17_C2, D_9_17_B2, -D_9_17_B2,
            D_9_17_C3, -D_9_17_C3, D_9_17_B3, -D_9_17_B3,
            D_9_17_C4, -D_9_17_C4, D_9_17_B4, -D_9_17_B4},
        new double[] {0.0,
            0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0},
        new double[] {D_9_17_W0,
            D_9_17_W1, D_9_17_W1, D_9_17_W1, D_9_17_W1,
            D_9_17_W2, D_9_17_W2, D_9_17_W2, D_9_17_W2,
            D_9_17_W3, D_9_17_W3, D_9_17_W3, D_9_17_W3,
            D_9_17_W4, D_9_17_W4, D_9_17_W4, D_9_17_W4}));

    private static CubatureRule crv3Pg = createTransformedCubeRule(new CubatureRule(
        "boxes degree 3, 8 positions, product Gauss formula",
        8,
        new double[] {D_3_8_U, D_3_8_U, D_3_8_U, D_3_8_U, -D_3_8_U, -D_3_8_U, -D_3_8_U, -D_3_8_U},
        new double[] {D_3_8_U, D_3_8_U, -D_3_8_U, -D_3_8_U, D_3_8_U, D_3_8_U, -D_3_8_U, -D_3_8_U},
        new double[] {D_3_8_U, -D_3_8_U, D_3_8_U, -D_3_8_U, D_3_8_U, -D_3_8_U, D_3_8_U, -D_3_8_U},
        new double[] {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0}));

    // quadprodrule[i-1] is a product rule of degree 2i+1 over [-1,1]^2
    private static final CubatureRule[] quadProductRule = {
        crq3Pg,
        crq5Pg,
        crq7Pg
    };

    // boxesprodrule[i-1] is a product rule of degree 2i+1 over [-1,1]^3
    private static final CubatureRule[] boxesProductRule = {crv3Pg};

    private static CubatureRule degree8QuadrilateralRule;
    private static CubatureRule degree1BoxRule;

    private static void transformQuadRuleInPlace(CubatureRule rule) {
        if (rule == null) {
            return;
        }
        for (int k = 0; k < rule.numberOfNodes; k++) {
            rule.u[k] = (rule.u[k] + 1.0) / 2.0;
            rule.v[k] = (rule.v[k] + 1.0) / 2.0;
            rule.w[k] /= 4.0;
        }
    }

    private static void transformCubeRuleInPlace(CubatureRule rule) {
        if (rule == null) {
            return;
        }
        for (int k = 0; k < rule.numberOfNodes; k++) {
            rule.u[k] = (rule.u[k] + 1.0) / 2.0;
            rule.v[k] = (rule.v[k] + 1.0) / 2.0;
            rule.t[k] = (rule.t[k] + 1.0) / 2.0;
            rule.w[k] /= 8.0;
        }
    }

    private static CubatureRule createTransformedQuadRule(CubatureRule rule) {
        transformQuadRuleInPlace(rule);
        return rule;
    }

    private static CubatureRule createTransformedCubeRule(CubatureRule rule) {
        transformCubeRuleInPlace(rule);
        return rule;
    }

    /**
    Degree 8, 16 positions from Wissman & Becker (cfr supra)

    We chose formula 8-2 on p 684 since it seems to have nicest weights and
    abscissa.

    The minimal number of nodes is 15, but the one known rule that achieves this
    minial number of nodes has nodes outside the unit square. That's not a
    desirable situation for us (see Cools & Rabinowitz ...).
    Btw, the formula of degree 9 has only one point more than this one.
    */
    public static CubatureRule degree8QuadrilateralRule() {
        if (degree8QuadrilateralRule == null) {
            degree8QuadrilateralRule = createTransformedQuadRule(new CubatureRule(
                "quads degree 8, 16 positions",
                16,
                new double[] {0.0, 0.0, 0.952509466071562, -0.952509466071562,
                    0.532327454074206, -0.532327454074206, 0.684736297951735, -0.684736297951735,
                    0.233143240801405, -0.233143240801405, 0.927683319306117, -0.927683319306117,
                    0.453120687403749, -0.453120687403749, 0.837503640422812, -0.837503640422812},
                new double[] {0.659560131960342, -0.949142923043125, 0.765051819557684, 0.765051819557684,
                    0.936975981088416, 0.936975981088416, 0.333656717735747, 0.333656717735747,
                    -0.079583272377397, -0.079583272377397, -0.272240080612534, -0.272240080612534,
                    -0.613735353398028, -0.613735353398028, -0.888477650535971, -0.888477650535971},
                new double[] {0.0, 0.0, 0.0, 0.0,
                    0.0, 0.0, 0.0, 0.0,
                    0.0, 0.0, 0.0, 0.0,
                    0.0, 0.0, 0.0, 0.0},
                new double[] {0.450276776305590, 0.166570426777813, 0.098869459933431, 0.098869459933431,
                    0.153696747140812, 0.153696747140812, 0.396686976072903, 0.396686976072903,
                    0.352014367945695, 0.352014367945695, 0.189589054577798, 0.189589054577798,
                    0.375101001147587, 0.375101001147587, 0.125618791640072, 0.125618791640072}));
        }
        return degree8QuadrilateralRule;
    }

    // Degree 1, 9 positions
    public static CubatureRule degree1BoxRule() {
        if (degree1BoxRule == null) {
            degree1BoxRule = createTransformedCubeRule(new CubatureRule(
                "boxes degree 1, 9 positions (the corners + center)",
                9,
                new double[] {D_1_9_U, D_1_9_U, D_1_9_U, D_1_9_U, -D_1_9_U, -D_1_9_U, -D_1_9_U, -D_1_9_U, 0.0},
                new double[] {D_1_9_U, D_1_9_U, -D_1_9_U, -D_1_9_U, D_1_9_U, D_1_9_U, -D_1_9_U, -D_1_9_U, 0.0},
                new double[] {D_1_9_U, -D_1_9_U, D_1_9_U, -D_1_9_U, D_1_9_U, -D_1_9_U, D_1_9_U, -D_1_9_U, 0.0},
                new double[] {D_1_9_W, D_1_9_W, D_1_9_W, D_1_9_W, D_1_9_W, D_1_9_W, D_1_9_W, D_1_9_W, D_1_9_W}));
        }
        return degree1BoxRule;
    }

    /**
    Installs cubature rules for triangles and quadrilaterals of the specified degree
    */
    public static void setQuadCubatureRules(CubatureRule[] quadRule, CubatureDegree degree) {
        switch (degree) {
            case DEGREE_1:
                quadRule[0] = crq1;
                break;
            case DEGREE_2:
                quadRule[0] = crq2;
                break;
            case DEGREE_3:
                quadRule[0] = crq3;
                break;
            case DEGREE_4:
                quadRule[0] = crq4;
                break;
            case DEGREE_5:
                quadRule[0] = crq5;
                break;
            case DEGREE_6:
                quadRule[0] = crq6;
                break;
            case DEGREE_7:
                quadRule[0] = crq7;
                break;
            case DEGREE_8:
                quadRule[0] = QuadCubatureRule.degree8QuadrilateralRule();
                break;
            case DEGREE_9:
                quadRule[0] = crq9;
                break;
            case DEGREE_3_PROD:
                quadRule[0] = crq3Pg;
                break;
            case DEGREE_5_PROD:
                quadRule[0] = crq5Pg;
                break;
            case DEGREE_7_PROD:
                quadRule[0] = crq7Pg;
                break;
            default:
                Error.fatal(2, "setQuadCubatureRules", "Invalid degree %d", degree.ordinal());
        }
    }
}
