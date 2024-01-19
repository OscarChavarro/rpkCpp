/*
cubic orthonormal basis for the unit square [0,1]^2
*/

#include "raycasting/stochasticRaytracing/basismcrad.h"

static double qm0(double u, double v) {
    return 1.000000000000000;
}

static double qm1(double u, double v) {
    return -1.732050807568877 + 3.464101615137753 * u;
}

static double qm2(double u, double v) {
    return -1.732050807568877 + 3.464101615137753 * v;
}

static double qm3(double u, double v) {
    return 3.000000000000003 + -6.000000000000006 * u + -6.000000000000009 * v + 12.000000000000021 * u * v;
}

static double qm4(double u, double v) {
    return 2.236067977499749 + -13.416407864998552 * u + 13.416407864998591 * u * u;
}

static double qm5(double u, double v) {
    return 2.236067977499781 + -13.416407864998723 * v + 13.416407864998760 * v * v;
}

static double qm6(double u, double v) {
    return -2.645751311064023 + 31.749015732770424 * u + -79.372539331927356 * u * u + 52.915026221285316 * u * u * u;
}

static double qm7(double u, double v) {
    return -3.872983346207165 + 23.237900077242056 * u + 7.745966692414697 * v + -46.475800154488844 * u * v +
           -23.237900077239200 * u * u + 46.475800154488617 * u * u * v;
}

static double qm8(double u, double v) {
    return -3.872983346207866 + 7.745966692416303 * u + 23.237900077246348 * v + -46.475800154495623 * u * v +
           -23.237900077245619 * v * v + 46.475800154491409 * u * v * v;
}

static double qm9(double u, double v) {
    return -2.645751311064409 + 31.749015732781054 * v + -79.372539331951486 * v * v + 52.915026221299712 * v * v * v;
}

static double (*f[MAX_BASIS_SIZE])(double, double) =
        {qm0, qm1, qm2, qm3, qm4, qm5, qm6, qm7, qm8, qm9}; // Functions

static FILTERTABLE h;  /* push-pull filter: computed in basis.c */

GalerkinBasis GLOBAL_stochasticRadiosisty_quadBasis = {
        "orthonormal basis on the unit square", // Description
        MAX_BASIS_SIZE, // Size
        f, f, // Primary and dual canonical basis functions are equal
        &h // Push-pull filter coefficients
};
