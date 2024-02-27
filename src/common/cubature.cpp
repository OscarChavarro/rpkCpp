/**
Note: a "cubature rule" is a numerical integration method used to approximate
integration of functions in several dimensions.

Good cubature rules of degree <= 9 for quadrilaterals and triangles
Philippe Bekaert - Department of Computer Science, K. U. Leuven (Belgium)
Philippe.Bekaert@cs.kuleuven.ac.be
September, 5 1995
*/

#include "common/cubature.h"

/**
quadrilaterals: [-1, 1] ^ 2
*/

/**
Degree 1, 1 point
*/
CUBARULE GLOBAL_crq1 = {
    "quads degree 1, 1 point",
    1,
    1,
    {0.0},
    {0.0},
    {0.0},
    {4.0}
};

/**
Degree 2, 3 positions Stroud '71
*/
#define w (4.0/3.0)
#define u 0.81649658092772603272 // sqrt(2/3)
#define c (-0.5) // cos(2*M_PI/3)
#define s 0.86602540378443864676 // sin(2*M_PI/3)
CUBARULE GLOBAL_crq2 = {
    "quads degree 2, 3 positions",
    2,
    3,
    {u, u * c, u * c},
    {0.0, u * s, -u * s},
    {0.0, 0.0, 0.0},
    {w, w, w}
};
#undef s
#undef c
#undef u
#undef w

/**
Degree 3, 4 positions, Davis & Rabinowitz, Methods of Numerical Integration,
2nd edition 1984, p 367
*/
#define u 0.81649658092772603272 // sqrt(2/3)
CUBARULE GLOBAL_crq3 = {
    "quads degree 3, 4 positions",
    3,
    4,
    {u, 0.0, -u, 0.0},
    {0.0, u, 0.0, -u},
    {0.0, 0.0, 0.0, 0.0},
    {1.0, 1.0, 1.0, 1.0}
};
#undef u

// Degree 3, 4 positions, product Gauss-Legendre formula
#define u 0.57735026918962576450 // sqrt(1/3)
CUBARULE GLOBAL_crq3pg = {
    "quads degree 3, 4 positions, product Gauss formula",
    3, // Degree
    4, // Positions
    {u, u, -u, -u}, // 1st coord. of abscissae
    {u, -u, u, -u}, // 2nd coord. of abscissae
    {0.0, 0.0, 0.0, 0.0}, // 3rd coord. of abscissae
    {1.0, 1.0, 1.0, 1.0} // Weights
};
#undef u

/**
Degree 4, 6 positions
see: Wissman, Becker, "Partially Symmetric Cubature Formulas for Even
Degrees of Exactness", SIAM. J. Numer. Anal., Vol 23 nr 3 (1986), p 676
You'll find also another similar rule in this paper, but I chose this one
because the abscissae seem to be nicer located.
You'll find the same rule in: Schmid, "On Cubature Formulae with a Minimal
Number of Knots", Numer. Math. Vol 31 (1978) p281
*/
CUBARULE GLOBAL_crq4 = {
    "quads degree 4, 6 positions",
    4, // Degree
    6, // Positions
    {0.0, 0.0, 0.774596669241483, -0.774596669241483, 0.774596669241483, -0.774596669241483},
    {-0.356822089773090, 0.934172358962716, 0.390885162530071, 0.390885162530071, -0.852765377881771, -0.852765377881771},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {1.286412084888852, 0.491365692888926, 0.761883709085613, 0.761883709085613, 0.349227402025498, 0.349227402025498}
};

// Degree 5, 7 positions, Radon's rule see e.g. Stroud '71
#define w1 (8.0 / 7.0)
#define w2 (5.0 / 9.0)
#define w3 (20.0 / 63.0)

// sqrt(14/15)
#define r 0.96609178307929588492

// sqrt(1/3)
#define s 0.57735026918962573106

// sqrt(3/5)
#define t 0.77459666924148340428
CUBARULE GLOBAL_crq5 = {
    "quads degree 5, 7 positions, Radon's rule",
    5, // Degree
    7, // Positions
    {0.0, s, s, -s, -s, r, -r},
    {0.0, t, -t, t, -t, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {w1, w2, w2, w2, w2, w3, w3}
};
#undef w1
#undef w2
#undef w3
#undef r
#undef s
#undef t

// Degree 5, 9 positions product Gauss-Legendre rule
// abscissae and weights computed using Stuff/gauleg.c
#define x0 0.0
#define w0 (8.0 / 9.0)
#define x1 0.7745966692414834
#define w1 (5.0 / 9.0)
CUBARULE GLOBAL_crq5pg = {
    "quads degree 5, 9 positions product Gauss rule",
    5,
    9,
    {-x1, -x1, -x1, x0, x0, x0, x1, x1, x1},
    {-x1, x0, x1, -x1, x0, x1, -x1, x0, x1},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {w1 * w1, w1 * w0, w1 * w1, w0 * w1, w0 * w0, w0 * w1, w1 * w1, w1 * w0, w1 * w1}
};
#undef w1
#undef x1
#undef w0
#undef x0

/**
Degree 6, 10 positions
from: Wissmann & Becker (cfr supra)
They again give two formulae of this type and you'll also find one
in Schmid, but I chose this one because it has the nicest weights
*/
CUBARULE GLOBAL_crq6 = {
    "quads degree 6, 10 positions",
    6, // Degree
    10, // Positions
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
#define r 0.92582009977255141919    /* sqrt(6./7.) */
#define s 0.38055443320831561227    /* sqrt((114.-3.*sqrt(583.))/287.) */
#define t 0.80597978291859884159    /* sqrt((114.+3.*sqrt(583.))/287.) */
#define w1 0.24197530864197530631    /* 49./810.*4. */
#define w2 0.52059291666739448967    /* (178981.+2769.*sqrt(583.))/1888920.*4. */
#define w3 0.23743177469063023177    /* (178981.-2769.*sqrt(583.))/1888920.*4. */
CUBARULE GLOBAL_crq7 = {
    "quads degree 7, 12 positions",
    7 /* degree */, 12 /* positions */,
    {r, -r, 0.0, 0.0, s, s, -s, -s, t, t, -t, -t},
    {0.0, 0.0, r, -r, s, -s, s, -s, t, -t, t, -t},
    {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.},
    {w1, w1, w1, w1, w2, w2, w2, w2, w3, w3, w3, w3}
};
#undef r
#undef s
#undef t
#undef w1
#undef w2
#undef w3

// Degree 7, 16 positions product Gauss rule
#define x1 0.86113631159405257522
#define x2 0.33998104358485626480
#define w1 0.34785484513745385737
#define w2 0.65214515486254614263
CUBARULE GLOBAL_crq7pg = {
    "quads degree 7, 16 positions product Gauss rule",
    7,
    16,
    {-x1, -x1, -x1, -x1, -x2, -x2, -x2, -x2, x2, x2, x2, x2, x1, x1, x1, x1},
    {-x1, -x2, x2, x1, -x1, -x2, x2, x1, -x1, -x2, x2, x1, -x1, -x2, x2, x1},
    {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.},
    {w1 * w1, w1 * w2, w1 * w2, w1 * w1, w2 * w1, w2 * w2, w2 * w2, w2 * w1, w2 * w1, w2 * w2, w2 * w2, w2 * w1,
     w1 * w1, w1 * w2, w1 * w2, w1 * w1}
};
#undef w2
#undef w1
#undef x2
#undef x1

/**
Degree 8, 16 positions from Wissman & Becker (cfr supra)

We chose formula 8-2 on p 684 since it seems to have nicest weights and
abscissae.

The minimal number of nodes is 15, but the one known rule that achieves this
minial number of nodes has nodes outside the unit square. That's not a
desirable situation for us (see Cools & Rabinowitz ...).
Btw, the formula of degree 9 has only one point more than this one.
*/
CUBARULE GLOBAL_crq8 = {
    "quads degree 8, 16 positions",
    8,
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

/**
Degree 9, 17 positions, Moeller, "Kubaturformeln mit minimaler Knotenzahl, Numer. Math. 25, 185 (1976)
*/
#define b1 0.96884996636197772072
#define b2 0.75027709997890053354
#define b3 0.52373582021442933604
#define b4 0.07620832819261717318
#define c1 0.63068011973166885417
#define c2 0.92796164595956966740
#define c3 0.45333982113564719076
#define c4 0.85261572933366230775
#define w0 0.52674897119341563786
#define w1 0.08887937817019870697
#define w2 0.11209960212959648528
#define w3 0.39828243926207009528
#define w4 0.26905133763978080301
CUBARULE GLOBAL_crq9 = {
    "quads degree 9, 17 positions",
    9,
    17,
    {0.0,
     b1, -b1, -c1, c1,
     b2, -b2, -c2, c2,
     b3, -b3, -c3, c3,
     b4, -b4, -c4, c4},
    {0.0,
     c1, -c1, b1, -b1,
     c2, -c2, b2, -b2,
     c3, -c3, b3, -b3,
     c4, -c4, b4, -b4},
    {0.0,
     0.0, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0},
    {w0,
     w1, w1, w1, w1,
     w2, w2, w2, w2,
     w3, w3, w3, w3,
     w4, w4, w4, w4}
};
#undef w4
#undef w3
#undef w2
#undef w1
#undef w0
#undef c4
#undef c3
#undef c2
#undef c1
#undef b4
#undef b3
#undef b2
#undef b1

/**
Triangles: barycentric coordinates
Weights sum to 1. instead of 0.5, which is the area of the triangle
0 <= x+y <= 1, x,y >= 0
*/

// Degree 1, 1 point
CUBARULE GLOBAL_crt1 = {
    "triangles degree 1, 1 positions",
    1,
    1,
    {1.0 / 3.0},
    {1.0 / 3.0},
    {0.0},
    {1.0}
};

/* degree 2, 3 positions,
 * Stroud '71 p 307 */
CUBARULE GLOBAL_crt2 = {
    "triangles degree 2, 3 positions",
    2,
    3,
    {1.0 / 6.0, 1.0 / 6.0, 2.0 / 3.0},
    {1.0 / 6.0, 2.0 / 3.0, 1.0 / 6.0},
    {0.0, 0.0, 0.0},
    {1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0}
};

// Degree 3, 4 positions, Stroud '71 p 308
CUBARULE GLOBAL_crt3 = {
    "triangles degree 3, 4 positions",
    3,
    4,
    {1.0 / 3.0, 0.2, 0.2, 0.6},
    {1.0 / 3.0, 0.2, 0.6, 0.2},
    {0.0, 0.0, 0.0, 0.0},
    {-9.0 / 16.0, 25.0 / 48.0, 25.0 / 48.0, 25.0 / 48.0}
};

/**
Degree 4, 6 positions
Lyness, Jespersen, "Moderate Degree Symmertic Quadrature Rules for the
Triangle", J. Inst. Maths. Applics (1975) 15, 19-32
*/
#define w1 (3.298552309659655e-1 / 3.0)
#define a1 8.168475729804585e-1
#define b1 9.157621350977073e-2
#define c1 b1
#define w2 (6.701447690340345e-1 / 3.0)
#define a2 1.081030181680702e-1
#define b2 4.459484909159649e-1
#define c2 b2
CUBARULE GLOBAL_crt4 = {
    "triangles degree 4, 6 positions",
    4, 6,
    {a1, b1, c1, a2, b2, c2},
    {b1, c1, a1, b2, c2, a2},
    {0., 0., 0., 0., 0., 0.},
    {w1, w1, w1, w2, w2, w2}
};
#undef c2
#undef b2
#undef a2
#undef w2
#undef c1
#undef b1
#undef a1
#undef w1

// Degree 5, 7 positions, Stroud '71 p 314
#define r 0.1012865073234563
#define s 0.7974269853530873
#define t (1.0 / 3.0)
#define u 0.4701420641051151
#define v 0.05971587178976981
#define A 0.225
#define B 0.1259391805448271
#define C 0.1323941527885062
CUBARULE GLOBAL_crt5 = {
    "triangles degree 5, 7 positions",
    5,
    7,
    {t, r, r, s, u, u, v},
    {t, r, s, r, u, v, u},
    {0., 0., 0., 0., 0., 0., 0.},
    {A, B, B, B, C, C, C}
};
#undef C
#undef B
#undef A
#undef v
#undef u
#undef t
#undef s
#undef r

/**
Degree 7, 12 nodes, Gaterman, "The Construction of Symmetric Cubature Formulas for the
Square and the triangle", Computing, 40, 229-240 (1988)
*/
#define w1 (0.2651702815743450e-1 * 2.0)
#define a1 0.6238226509439084e-1
#define b1 0.6751786707392436e-1
#define c1 0.8700998678316848
#define w2 (0.4388140871444811e-1 * 2.0)
#define a2 0.5522545665692000e-1
#define b2 0.3215024938520156
#define c2 0.6232720494910644
#define w3 (0.2877504278497528e-1 * 2.0)
#define a3 0.3432430294509488e-1
#define b3 0.6609491961867980
#define c3 0.3047265008681072
#define w4 (0.6749318700980879e-1 * 2.0)
#define a4 0.5158423343536001
#define b4 0.2777161669764050
#define c4 0.2064414986699949
CUBARULE GLOBAL_crt7 = {
    "triangles degree 7, 12 positions",
    7, 12,
    {a1, b1, c1, a2, b2, c2,
     a3, b3, c3, a4, b4, c4},
    {b1, c1, a1, b2, c2, a2,
     b3, c3, a3, b4, c4, a4},
    {0., 0., 0., 0., 0., 0.,
     0., 0., 0., 0., 0., 0.},
    {w1, w1, w1, w2, w2, w2,
     w3, w3, w3, w4, w4, w4}
};
#undef c4
#undef b4
#undef a4
#undef w4
#undef c3
#undef b3
#undef a3
#undef w3
#undef c2
#undef b2
#undef a2
#undef w2
#undef c1
#undef b1
#undef a1
#undef w1

// Degree 8, 16 positions Lyness & Jespersen
#define w0 1.443156076777862e-1
#define a0 3.333333333333333e-1
#define b0 3.333333333333333e-1
#define c0 b0
#define w1 (2.852749028018549e-1 / 3.0)
#define a1 8.141482341455413e-2
#define b1 4.592925882927229e-1
#define c1 b1
#define w2 (9.737549286959440e-2 / 3.0)
#define a2 8.989055433659379e-1
#define b2 5.054722831703103e-2
#define c2 b2
#define w3 (3.096521116041552e-1 / 3.0)
#define a3 6.588613844964797e-1
#define b3 1.705693077517601e-1
#define c3 b3
#define w4 (1.633818850466092e-1 / 6.0)
#define a4 8.394777409957211e-3
#define b4 7.284923929554041e-1
#define c4 2.631128296346387e-1
CUBARULE GLOBAL_crt8 = {
    "triangles degree 8, 16 positions",
    8,
    16,
    {a0,
     a1, b1, c1,
     a2, b2, c2,
     a3, b3, c3,
     a4, a4, b4, b4, c4, c4},
    {b0,
     b1, c1, a1,
     b2, c2, a2,
     b3, c3, a3,
     b4, c4, c4, a4, a4, b4},
    {0.0,
     0.0, 0.0, 0.0,
     0.0, 0.0, 0.0,
     0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {w0,
     w1, w1, w1,
     w2, w2, w2,
     w3, w3, w3,
     w4, w4, w4, w4, w4, w4}
};
#undef c4
#undef b4
#undef a4
#undef w4
#undef c3
#undef b3
#undef a3
#undef w3
#undef c2
#undef b2
#undef a2
#undef w2
#undef c1
#undef b1
#undef a1
#undef w1
#undef c0
#undef b0
#undef a0
#undef w0

/**
Degree 9, 19 positions - there is as yet no rule over the triangle known
which has only the minimal number of 17 nodes (see Cools & Rabinowitz).
Lyness & Jespersen
*/
#define w0 9.713579628279610e-2
#define a0 3.333333333333333e-1
#define b0 3.333333333333333e-1
#define c0 b0
#define w1 (9.400410068141950e-2 / 3.0)
#define a1 2.063496160252593e-2
#define b1 4.896825191987370e-1
#define c1 b1
#define w2 (2.334826230143263e-1 / 3.0)
#define a2 1.258208170141290e-1
#define b2 4.370895914929355e-1
#define c2 b2
#define w3 (2.389432167816273e-1 / 3.0)
#define a3 6.235929287619356e-1
#define b3 1.882035356190322e-1
#define c3 b3
#define w4 (7.673302697609430e-2 / 3.0)
#define a4 9.105409732110941e-1
#define b4 4.472951339445297e-2
#define c4 b4
#define w5 (2.597012362637364e-1 / 6.0)
#define a5 3.683841205473626e-2
#define b5 7.411985987844980e-1
#define c5 2.219629891607657e-1
CUBARULE GLOBAL_crt9 = {
    "triangles degree 9, 19 positions",
    9, 19,
    {a0,
     a1, b1, c1,
     a2, b2, c2,
     a3, b3, c3,
     a4, b4, c4,
     a5, a5, b5, b5, c5, c5},
    {b0,
     b1, c1, a1,
     b2, c2, a2,
     b3, c3, a3,
     b4, c4, a4,
     b5, c5, c5, a5, a5, b5},
    {0.0,
     0.0, 0.0, 0.0,
     0.0, 0.0, 0.0,
     0.0, 0.0, 0.0,
     0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {w0,
     w1, w1, w1,
     w2, w2, w2,
     w3, w3, w3,
     w4, w4, w4,
     w5, w5, w5, w5, w5, w5}
};
#undef c5
#undef b5
#undef a5
#undef w5
#undef c4
#undef b4
#undef a4
#undef w4
#undef c3
#undef b3
#undef a3
#undef w3
#undef c2
#undef b2
#undef a2
#undef w2
#undef c1
#undef b1
#undef a1
#undef w1
#undef c0
#undef b0
#undef a0
#undef w0

/**
Boxes: [-1, 1] ^ 3
*/

// Degree 1, 9 positions
#define u 1.0
#define w (8.0 / 9.0)
CUBARULE GLOBAL_crv1 = {
    "boxes degree 1, 9 positions (the corners + center)",
    1, // Degree
    9, // Positions
    {u, u, u, u, -u, -u, -u, -u, 0.0},
    {u, u, -u, -u, u, u, -u, -u, 0.0},
    {u, -u, u, -u, u, -u, u, -u, 0.0},
    {w, w, w, w, w, w, w, w, w}
};
#undef u
#undef w

// Degree 3, 8 positions, product Gauss-Legendre formula
// sqrt(1.0 / 3.0)
#define u 0.57735026918962576450
CUBARULE GLOBAL_crv3pg = {
    "boxes degree 3, 8 positions, product Gauss formula",
    3 /* degree */, 8 /* positions */,
    {u, u, u, u, -u, -u, -u, -u},
    {u, u, -u, -u, u, u, -u, -u},
    {u, -u, u, -u, u, -u, u, -u},
    {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0}
};
#undef u

// quadrule[i-1] is a rule of degree = i (i=1..9) over [-1,1]^2
static CUBARULE *globalQuadRule[9] = {&GLOBAL_crq1, &GLOBAL_crq2, &GLOBAL_crq3, &GLOBAL_crq4, &GLOBAL_crq5, &GLOBAL_crq6, &GLOBAL_crq7, &GLOBAL_crq8, &GLOBAL_crq9};

// quadprodrule[i-1] is a product rule of degree 2i+1 over [-1,1]^2
static CUBARULE *globalQuadProductRule[3] = {&GLOBAL_crq3pg, &GLOBAL_crq5pg, &GLOBAL_crq7pg};

// boxesrule[i-1] is a degree i rule over [-1,1] ^ 3
static CUBARULE *globalBoxesRule[1] = {&GLOBAL_crv1};

// boxesprodrule[i-1] is a product rule of degree 2i+1 over [-1,1]^3
static CUBARULE *globalBoxesProductRule[1] = {&GLOBAL_crv3pg};

/**
This routine transforms a rule over [-1,1]^2 to the unit square [0,1]^2
*/
static void
cubatureTransformQuadRule(CUBARULE *rule) {
    for ( int k = 0; k < rule->numberOfNodes; k++ ) {
        rule->u[k] = (rule->u[k] + 1.0) / 2.0;
        rule->v[k] = (rule->v[k] + 1.0) / 2.0;
        rule->w[k] /= 4.0;
    }
}

/**
This routine transforms a rule over [-1,1]^3 to the unit cube [0,1]^3
*/
static void
cubatureTransformCubeRule(CUBARULE *rule) {
    for ( int k = 0; k < rule->numberOfNodes; k++ ) {
        rule->u[k] = (rule->u[k] + 1.) / 2.;
        rule->v[k] = (rule->v[k] + 1.) / 2.;
        rule->t[k] = (rule->t[k] + 1.) / 2.;
        rule->w[k] /= 8.;
    }
}

/**
This routine should be called during initialization of the program: it
transforms the rules over [-1,1]^2 to rules over the unit square [0,1]^2,
which we use to map to patches
*/
void
fixCubatureRules() {
    int i;

    for ( i = 0; i < 9; i++ ) {
        cubatureTransformQuadRule(globalQuadRule[i]);
    }
    for ( i = 0; i < 3; i++ ) {
        cubatureTransformQuadRule(globalQuadProductRule[i]);
    }
    for ( i = 0; i < 1; i++ ) {
        cubatureTransformCubeRule(globalBoxesRule[i]);
    }
    for ( i = 0; i < 1; i++ ) {
        cubatureTransformCubeRule(globalBoxesProductRule[i]);
    }
}
