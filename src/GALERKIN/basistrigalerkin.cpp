/**
Orthonormal basis for the standard triangle (0, 0), (1, 0), (0, 1)
*/
#include "GALERKIN/basisgalerkin.h"

static double tg0(double /*u*/, double /*v*/) {
    return 1.000000000000000;
}

static double tg1(double u, double /*v*/) {
    return -1.414213562373095 * 1 + 4.242640687119287 * u;
}

static double tg2(double u, double v) {
    return -2.449489742783179 * 1 + 2.449489742783180 * u + 4.898979485566360 * v;
}

static double tg3(double u, double v) {
    return 1.133893419027696 * 1 + -4.535573676110755 * u + -4.535573676110757 * v + 22.677868380553690 * u * v;
}

static double tg4(double u, double v) {
    return 3.273268353539930 * 1 + -22.258224804071368 * u + -3.927922024247956 * v + 19.639610121239613 * u * v +
           22.912878474779255 * u * u;
}

static double tg5(double u, double v) {
    return 3.872983346207630 * 1 + -7.745966692415757 * u + -23.237900077245097 * v + 23.237900077245847 * u * v +
           3.872983346208171 * u * u + 23.237900077244831 * v * v;
}

static double tg6(double u, double v) {
    return -1.999999999999970 * 1 + 30.000000000000028 * u + 0.000000000000188 * v + 0.000000000000721 * u * v +
           -90.000000000002487 * u * u + -0.000000000000351 * v * v + 70.000000000003141 * u * u * u;
}

static double tg7(double u, double v) {
    return -3.464101615137785 * 1 + 45.033320996788269 * u + 6.928203230276860 * v + -83.138438763305757 * u * v +
           -114.315353299539055 * u * u + -0.000000000001427 * v * v + 72.746133917888613 * u * u * u +
           145.492267835783281 * u * u * v;
}

static double tg8(double u, double v) {
    return -4.472135955000763 * 1 + 40.249223595002263 * u + 26.832815730003585 * v + -214.662525840012592 * u * v +
           -67.082039324999926 * u * u + -26.832815730002483 * v * v + 31.304951684997551 * u * u * u +
           187.829710110013110 * u * u * v + 187.829710110001940 * u * v * v;
}

static double tg9(double u, double v) {
    return -5.291502622131427 * 1 + 15.874507866401922 * u + 63.498031465565624 * v + -126.996062931158960 * u * v +
           -15.874507866410964 * u * u + -158.745078663906781 * v * v + 5.291502622139829 * u * u * u +
           63.498031465601095 * u * u * v + 158.745078663922413 * u * v * v + 105.830052442603559 * v * v * v;
}

GalerkinBasis GLOBAL_galerkin_triBasis = {
    "orthonormal basis for the standard triangle", // Description
    10, // Size
    {tg0, tg1, tg2, tg3, tg4, tg5, tg6, tg7, tg8, tg9} // Functions
};
