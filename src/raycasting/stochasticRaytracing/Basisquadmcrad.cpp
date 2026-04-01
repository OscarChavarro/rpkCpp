/**
Cubic orthonormal basis for the unit square [0, 1] ^ 2
*/

#include "raycasting/stochasticRaytracing/Basismcrad.h"

class Basisquadmcrad final {
  public:
    static double qm0(double u, double v);
    static double qm1(double u, double v);
    static double qm2(double u, double v);
    static double qm3(double u, double v);
    static double qm4(double u, double v);
    static double qm5(double u, double v);
    static double qm6(double u, double v);
    static double qm7(double u, double v);
    static double qm8(double u, double v);
    static double qm9(double u, double v);
};

double Basisquadmcrad::qm0(double /*u*/, double /*v*/) {
    return 1.000000000000000;
}

double Basisquadmcrad::qm1(double u, double /*v*/) {
    return -1.732050807568877 + 3.464101615137753 * u;
}

double Basisquadmcrad::qm2(double /*u*/, double v) {
    return -1.732050807568877 + 3.464101615137753 * v;
}

double Basisquadmcrad::qm3(double u, double v) {
    return 3.000000000000003 + -6.000000000000006 * u + -6.000000000000009 * v + 12.000000000000021 * u * v;
}

double Basisquadmcrad::qm4(double u, double /*v*/) {
    return 2.236067977499749 + -13.416407864998552 * u + 13.416407864998591 * u * u;
}

double Basisquadmcrad::qm5(double /*u*/, double v) {
    return 2.236067977499781 + -13.416407864998723 * v + 13.416407864998760 * v * v;
}

double Basisquadmcrad::qm6(double u, double /*v*/) {
    return -2.645751311064023 + 31.749015732770424 * u + -79.372539331927356 * u * u + 52.915026221285316 * u * u * u;
}

double Basisquadmcrad::qm7(double u, double v) {
    return -3.872983346207165 + 23.237900077242056 * u + 7.745966692414697 * v + -46.475800154488844 * u * v +
           -23.237900077239200 * u * u + 46.475800154488617 * u * u * v;
}

double Basisquadmcrad::qm8(double u, double v) {
    return -3.872983346207866 + 7.745966692416303 * u + 23.237900077246348 * v + -46.475800154495623 * u * v +
           -23.237900077245619 * v * v + 46.475800154491409 * u * v * v;
}

double Basisquadmcrad::qm9(double /*u*/, double v) {
    return -2.645751311064409 + 31.749015732781054 * v + -79.372539331951486 * v * v + 52.915026221299712 * v * v * v;
}

static double (*f[MAX_BASIS_SIZE])(double, double) =
        {Basisquadmcrad::qm0, Basisquadmcrad::qm1, Basisquadmcrad::qm2, Basisquadmcrad::qm3, Basisquadmcrad::qm4,
         Basisquadmcrad::qm5, Basisquadmcrad::qm6, Basisquadmcrad::qm7, Basisquadmcrad::qm8, Basisquadmcrad::qm9}; // Functions

static FILTER_TABLE h;  /* push-pull filter: computed in basis.c */

GalerkinBasis
stochasticRadiosityCreateQuadBasis() {
    return {
        "orthonormal basis on the unit square", // Description
        MAX_BASIS_SIZE, // Size
        f, f, // Primary and dual canonical basis functions are equal
        &h // Push-pull filter coefficients
    };
}
